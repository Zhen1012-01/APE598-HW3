#include <errno.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

static int N, stride;
static size_t inner_cells, padded_cells, domain_count;
static int *grid;
static int *domain_pos;
static uint64_t sweeps, processed_cells, topples;

static void die(const char *s) {
  fprintf(stderr, "%s\n", s);
  exit(EXIT_FAILURE);
}

static int in_domain(int i, int j) {
  int64_t x = (int64_t)i - N / 2;
  int64_t y = (int64_t)j - N / 2;
  int64_t r = N / 2;
  int maxj = i > N - 1 - i ? i : N - 1 - i;
  return j <= maxj && x * x + y * y <= r * r;
}

static void build_geometry(void) {
  domain_pos = malloc(inner_cells * sizeof(*domain_pos));
  if (!domain_pos) die("malloc failed for domain positions");
  domain_count = 0;
  for (int i = 0; i < N; ++i)
    for (int j = 0; j < N; ++j)
      if (in_domain(i, j))
        domain_pos[domain_count++] = (i + 1) * stride + (j + 1);
}

static void initialize_six(void) {
  for (int i = 0; i < N; ++i) {
    int *row = grid + (size_t)(i + 1) * stride + 1;
    for (int j = 0; j < N; ++j) row[j] = 6;
  }
}

static void invert_inner(void) {
  for (int i = 0; i < N; ++i) {
    int *row = grid + (size_t)(i + 1) * stride + 1;
    for (int j = 0; j < N; ++j) row[j] = 6 - row[j];
  }
}

static void stabilize(void) {
  int changed;
  do {
    changed = 0;
    sweeps++;
    if (sweeps & 1) {
      for (size_t k = 0; k < domain_count; ++k) {
        int p = domain_pos[k];
        int v = grid[p];
        if (v < 4) continue;
        int q = v >> 2;
        grid[p] = v - (q << 2);
        grid[p - stride] += q;
        grid[p - 1] += q;
        grid[p + stride] += q;
        grid[p + 1] += q;
        processed_cells++;
        topples += (uint64_t)q;
        changed = 1;
      }
    } else {
      for (size_t k = domain_count; k-- > 0;) {
        int p = domain_pos[k];
        int v = grid[p];
        if (v < 4) continue;
        int q = v >> 2;
        grid[p] = v - (q << 2);
        grid[p - stride] += q;
        grid[p - 1] += q;
        grid[p + stride] += q;
        grid[p + 1] += q;
        processed_cells++;
        topples += (uint64_t)q;
        changed = 1;
      }
    }
  } while (changed);
}

static void find_identity(void) {
  initialize_six();
  stabilize();
  invert_inner();
  stabilize();
}

static void maybe_dump(void) {
  const char *path = getenv("SANDPILE_DUMP");
  if (!path || !*path) return;
  FILE *f = fopen(path, "wb");
  if (!f) die("failed to open dump path");
  for (int i = 0; i < N; ++i) {
    int *row = grid + (size_t)(i + 1) * stride + 1;
    if (fwrite(row, sizeof(*row), (size_t)N, f) != (size_t)N)
      die("failed to write dump");
  }
  if (fclose(f)) die("failed to close dump");
}

static void render(const char *filename) {
  const int chan = 3;
  if (inner_cells > SIZE_MAX / chan) die("image size overflow");
  unsigned char *pixels = malloc(inner_cells * chan);
  if (!pixels) die("malloc failed for PNG buffer");
  size_t out = 0;
  for (int i = 0; i < N; ++i) {
    int *row = grid + (size_t)(i + 1) * stride + 1;
    for (int j = 0; j < N; ++j, ++out) {
      unsigned char r = 0, g = 0, b = 0;
      switch (row[j]) {
        case 0: r = 15; g = 15; b = 25; break;
        case 1: r = 0; g = 210; b = 255; break;
        case 2: r = 230; g = 30; b = 130; break;
        case 3: r = 255; g = 200; b = 0; break;
        default: r = g = b = 0;
      }
      pixels[3 * out] = r;
      pixels[3 * out + 1] = g;
      pixels[3 * out + 2] = b;
    }
  }
  if (!stbi_write_png(filename, N, N, chan, pixels, N * chan))
    die("stbi_write_png failed");
  printf("Successfully saved sandpile identity to: %s\n", filename);
  free(pixels);
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
      parsed > INT32_MAX - 2)
    die("grid size must be a positive int");
  N = (int)parsed;
  stride = N + 2;
  if ((size_t)N > SIZE_MAX / (size_t)N) die("grid size overflow");
  inner_cells = (size_t)N * N;
  if ((size_t)stride > SIZE_MAX / (size_t)stride ||
      (size_t)stride * stride > SIZE_MAX / sizeof(*grid))
    die("padded grid size overflow");
  padded_cells = (size_t)stride * stride;
  grid = calloc(padded_cells, sizeof(*grid));
  if (!grid) die("calloc failed for padded grid");
  build_geometry();
  find_identity();
  maybe_dump();
  render("identity.png");
  if (getenv("SANDPILE_STATS"))
    fprintf(stderr, "domain=%zu sweeps=%" PRIu64
                    " processed=%" PRIu64 " topples=%" PRIu64 "\n",
            domain_count, sweeps, processed_cells, topples);
  free(domain_pos);
  free(grid);
  return EXIT_SUCCESS;
}
