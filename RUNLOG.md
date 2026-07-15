# Run Log

| Profile | Delay (ms) | Miss Rate | Overhead | Result | Change |
|---|---:|---:|---:|---|---|
| A | 40 | 8.27% | 1.02x | INVALID | Naive baseline |
| B | 40 | 71.47% | 1.02x | INVALID | Naive baseline |
| A | 100 | 2.27% | 1.02x | INVALID | Increased delay only |
| B | 100 | 5.40% | 1.02x | INVALID | Increased delay only |
| A | 100 | 0.27% | 1.29x | VALID | XOR FEC, 4 data + 1 parity |
| B | 100 | 3.80% | 1.29x | INVALID | XOR FEC insufficient for multiple/burst losses |
| B | 100 | 0.87% | 1.55x | VALID | XOR FEC, 2 data + 1 parity |
| B | 80 | 3.07% | 1.55x | INVALID | Reduced playout delay |
| B | 90 | >1% | 1.55x | INVALID | Reduced playout delay |
| B | 95 | >1% | 1.55x | INVALID | Reduced playout delay |
| B | 100 | 0.80% | 1.55x | VALID | Reliability verification |

The baseline failed because packet loss and excessive network delay directly caused deadline misses. Increasing delay alone improved performance but could not bring misses below 1%.

I introduced XOR forward error correction (FEC). Initially, one parity packet was generated for every four data frames. This made Profile A valid but was insufficient for Profile B.

The FEC group was then reduced to two data frames plus one parity packet. The receiver stores received frames and parity information and reconstructs a missing frame when exactly one frame in an FEC group is lost.

The final selected playout delay is 100 ms. Attempts at 80, 90, and 95 ms were invalid on Profile B.
