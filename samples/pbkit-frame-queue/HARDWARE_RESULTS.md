# pbkit frame-queue validation results

Tested on 2026-11-20 on one original Xbox, with the same console, compiler,
video path and workload for every stock/patched comparison.

## Revisions and binaries

- Stock nxdk: `fafbe41b48fa00a31b79dd49ad1f86a30adcdd9e`
- Patched nxdk: `fbc6192464825172dad193b007a3c47e3eccf5d8`
- QA sample: `eb0b988` plus the dashboard-return change in the following QA
  commit
- Compiler: clang 22.1.8
- 480p stock SHA-256:
  `c199fc630220c922efd2389c33debefbf564de605858fad0c4ae02377d3eb375`
- 480p patched SHA-256:
  `bba25c3a622b55292220ff4a3375834d733a109c41acfdbed200a8c4126f67a7`
- 720p stock SHA-256:
  `c91f52a16ca45bef388d0239c4e9532b5c33491c6de4d1a101b01e930dc45d76`
- 720p patched SHA-256:
  `e9cea4a3676ce45ac536a523fc787b89df3d783050bd51c32d254e57d4bb1a18`

The sample was built from a clean directory for every variant:

```sh
eval "$(../../bin/activate -s)"
make clean && make VARIANT=stock MODE=480p REFRESHES=3600
make clean && make VARIANT=patched MODE=480p REFRESHES=3600
make clean && make VARIANT=stock MODE=720p REFRESHES=3600
make clean && make VARIANT=patched MODE=720p REFRESHES=3600
```

Each XBE was copied to a separate private QA game folder and launched from
XBMC with `SITE RunXBE(F:\\folder\\default.xbe)`. The resulting CSV was
retrieved before the next launch and checked with:

```sh
python analyze.py pbkit-queue-<variant>-<mode>-NN.csv
```

## Original Xbox: 480p

The order was stock (two launches), patched (two launches), then stock again
(two launches). Each launch requested 3,600 rendered frames and ran for 3,604
refreshes. All six runs recorded 3,602 queue events, 3,602 removals, zero
premature removals, zero repeated selections, zero deferrals and zero trace
overflows.

| Run | Stall episodes | Total stall (us) | Maximum stall (us) | Selection interval p95 / max (us) |
| --- | ---: | ---: | ---: | ---: |
| stock A1 | 3,599 | 53,324,140 | 14,871 | 16,666 / 16,667 |
| stock A2 | 3,599 | 53,321,429 | 14,869 | 16,666 / 16,667 |
| patched B1 | 3,599 | 53,164,633 | 14,826 | 16,666 / 16,667 |
| patched B2 | 3,599 | 53,161,274 | 14,828 | 16,666 / 16,667 |
| stock C1 | 3,599 | 53,321,222 | 14,869 | 16,666 / 16,667 |
| stock C2 | 3,599 | 53,324,061 | 14,872 | 16,666 / 16,667 |

Stock did not reproduce the premature-removal bug on this console in these
runs. No stalls or frame-pacing regression appeared in the patched runs.

## Original Xbox: 720p

The order was stock, patched, stock. Each launch requested 3,600 rendered
frames and ran for 3,604 refreshes. All three runs recorded 3,603 queue events,
3,603 removals, zero premature removals, zero repeated selections, zero
deferrals and zero trace overflows.

| Run | Stall episodes | Total stall (us) | Maximum stall (us) | Selection interval p95 / max (us) |
| --- | ---: | ---: | ---: | ---: |
| stock A | 3,599 | 51,871,821 | 14,469 | 16,666 / 16,667 |
| patched B | 3,600 | 51,719,821 | 14,426 | 16,666 / 16,667 |
| stock C | 3,600 | 51,881,329 | 14,468 | 16,666 / 16,667 |

Stock again did not reproduce the premature-removal bug. The one-episode stall
count difference was not accompanied by a duration or pacing regression.

## xemu ordering witness

xemu 0.8.139 (`aa54c34`) supplied a deterministic ordering witness. The stock
run queued frame 127 in surface 1, repeated frame 126 for one refresh, then
selected frame 128; frame 127 was removed without ever being selected. A
shortened trace is:

```text
queue frame=127 surface=1
ready frame=127 surface=1
select frame=126 surface=0  # repeated refresh; normal by itself
remove frame=127 surface=1 # premature: surface 1 was never selected
queue frame=128 surface=2
ready frame=128 surface=2
select frame=128 surface=2
```

The patched run selected and removed all 102 queued frames in order, with zero
premature removals. The stock xemu run recorded 101 frames / 2,103 events; the
patched run recorded 102 frames / 2,123 events and 232 scanout samples.

## GoldenEye cross-check

The isolated GoldenEye cross-check is tracked separately because queue
correctness and gameplay performance must be reported independently. Its first
stock hardware launch did not reach the file-select screen and remained black,
so it is not counted as evidence. Results remain pending.
