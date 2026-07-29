# Caedral C SDK

Official C client for the [Caedral API](https://caedral.com) (**v1.0.0**).

libcurl + vendored cJSON, ships as static library `libcaedral.a`.

## Build

```bash
# Requires: C11 compiler, libcurl development headers
git clone https://github.com/trycaedral/caedral-c.git
cd caedral-c
git checkout v1.0.0
make
# or: cmake -B build && cmake --build build
```

## Link

```bash
gcc -o myapp myapp.c -I./include -L./build -lcaedral -lcurl
```

## Quickstart

See `examples/` in this repository and https://caedral.com/docs/c.

Default API base URL: `https://api.caedral.com`. Keys use the `cd_live_` prefix.

## Models

**Chat:** `caedral-base` · `caedral-titan` · `caedral-olympus` · `caedral-primordial`  
**Specialized:** `caedral-vision` · `caedral-embed` · `caedral-voice` · `caedral-rerank`

## License

MIT
