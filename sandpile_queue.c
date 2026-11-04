#include <errno.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

static int N;
static size_t cells;
static int *grid;
static unsigned char *domain;
static uint64_t processed_cells;
static uint64_t batch_topples;

static void die(const char *s) {
  fprintf(stderr, "%s\n", s);
  exit(EXIT_FAILURE);
}

static int in_domain_coord(int i, int j) {
  int64_t x = (int64_t)i - N / 2;
  int64_t y = (int64_t)j - N / 2;
  int64_t r = N / 2;
  int maxj = i > N - 1 - i ? i : N - 1 - i;
  return j <= maxj && x * x + y * y <= r * r;
}

static void build_domain(void) {
  domain = calloc(cells, 1);
  if (!domain) die("calloc failed for domain mask");
  for (int i = 0; i < N; ++i)
    for (int j = 0; j < N; ++j)
      domain[(size_t)i * N + j] = (unsigned char)in_domain_coord(i, j);
}

static void add_neighbor(size_t p, int q, unsigned char *queued,
                         int *queue, size_t cap, size_t *tail, size_t *count) {
  grid[p] += q;
#ifndef SCAN_BATCH
  if (domain[p] && grid[p] >= 4 && !queued[p]) {
    if (*count == cap) die("active queue capacity exceeded");
    queue[*tail] = (int)p;
    *tail = (*tail + 1) % cap;
    (*count)++;
    queued[p] = 1;
  }
#else
  (void)queued; (void)queue; (void)cap; (void)tail; (void)count;
#endif
}

#ifndef SCAN_BATCH
static void stabilize(void) {
  unsigned char *queued = calloc(cells, 1);
  int *queue = malloc(cells * sizeof(*queue));
  if (!queued || !queue) die("allocation failed for active queue");
  size_t head = 0, tail = 0, count = 0;
  for (size_t p = 0; p < cells; ++p) {
    if (domain[p] && grid[p] >= 4) {
      queue[tail] = (int)p;
      tail = (tail + 1) % cells;
      count++;
      queued[p] = 1;
    }
  }
  while (count) {
    size_t p = (size_t)queue[head];
    head = (head + 1) % cells;
    count--;
    queued[p] = 0;
    int v = grid[p];
    if (v < 4) continue;
    int q = v / 4;
    grid[p] = v - 4 * q;
    processed_cells++;
    batch_topples += (uint64_t)q;
    int i = (int)(p / (size_t)N);
    int j = (int)(p - (size_t)i * N);
    if (i > 0) add_neighbor(p - N, q, queued, queue, cells, &tail, &count);
    if (j > 0) add_neighbor(p - 1, q, queued, queue, cells, &tail, &count);
    if (i + 1 < N) add_neighbor(p + N, q, queued, queue, cells, &tail, &count);
    if (j + 1 < N) add_neighbor(p + 1, q, queued, queue, cells, &tail, &count);
  }
  free(queue);
  free(queued);
}
#else
static void stabilize(void) {
  int changed;
  do {
    changed = 0;
    for (size_t p = 0; p < cells; ++p) {
      if (!domain[p] || grid[p] < 4) continue;
      int q = grid[p] / 4;
      grid[p] -= 4 * q;
      changed = 1;
      processed_cells++;
      batch_topples += (uint64_t)q;
      int i = (int)(p / (size_t)N);
      int j = (int)(p - (size_t)i * N);
      if (i > 0) grid[p - N] += q;
      if (j > 0) grid[p - 1] += q;
      if (i + 1 < N) grid[p + N] += q;
      if (j + 1 < N) grid[p + 1] += q;
    }
  } while (changed);
}
#endif

static void find_identity(void) {
  for (size_t p = 0; p < cells; ++p) grid[p] = 6;
  stabilize();
  for (size_t p = 0; p < cells; ++p) grid[p] = 6 - grid[p];
  stabilize();
}

static void render(const char *filename) {
  const int chan = 3;
  if (cells > SIZE_MAX / (size_t)chan) die("image size overflow");
  unsigned char *pixels = malloc(cells * chan);
  if (!pixels) die("malloc failed for PNG pixel buffer");
  for (size_t p = 0; p < cells; ++p) {
    unsigned char r = 0, g = 0, b = 0;
    switch (grid[p]) {
      case 0: r = 15; g = 15; b = 25; break;
      case 1: r = 0; g = 210; b = 255; break;
      case 2: r = 230; g = 30; b = 130; break;
      case 3: r = 255; g = 200; b = 0; break;
      default: r = g = b = 0;
    }
    pixels[3 * p] = r;
    pixels[3 * p + 1] = g;
    pixels[3 * p + 2] = b;
  }
  if (!stbi_write_png(filename, N, N, chan, pixels, N * chan))
    die("stbi_write_png failed");
  printf("Successfully saved sandpile identity to: %s\n", filename);
  free(pixels);
}

static void maybe_dump(void) {
  const char *path = getenv("SANDPILE_DUMP");
  if (!path || !*path) return;
  FILE *f = fopen(path, "wb");
  if (!f) die("failed to open dump path");
  if (fwrite(grid, sizeof(*grid), cells, f) != cells) die("failed to write dump");
  if (fclose(f)) die("failed to close dump");
}

int main(int argc, char **argv) {
  if (argc != 2) {
    fprintf(stderr, "Usage: %s N\n", argv[0]);
    return EXIT_FAILURE;
  }
  errno = 0;
  char *end = NULL;
  uintmax_t parsed = strtoumax(argv[1], &end, 10);
  if (errno || argv[1][0] < '0' || argv[1][0] > '9' || *end || !parsed ||
      parsed > INT32_MAX)
    die("grid size must be a positive int");
  N = (int)parsed;
  if ((size_t)N > SIZE_MAX / (size_t)N ||
      (size_t)N * (size_t)N > SIZE_MAX / sizeof(*grid))
    die("grid size is too large");
  cells = (size_t)N * (size_t)N;
  grid = malloc(cells * sizeof(*grid));
  if (!grid) die("malloc failed for grid");
  build_domain();
  find_identity();
  maybe_dump();
  render("identity.png");
  if (getenv("SANDPILE_STATS"))
    fprintf(stderr, "processed_cells=%" PRIu64 " topples=%" PRIu64 "\n",
            processed_cells, batch_topples);
  free(domain);
  free(grid);
  return EXIT_SUCCESS;
}
