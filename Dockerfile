FROM ubuntu:26.04
RUN apt-get update && apt-get install -y --no-install-recommends build-essential python3 valgrind time && rm -rf /var/lib/apt/lists/*
WORKDIR /artifact
COPY . .
RUN make all variants
CMD ["./sandpile.exe","1001"]
