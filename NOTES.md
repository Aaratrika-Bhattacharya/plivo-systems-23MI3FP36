The system uses proactive XOR-based Forward Error Correction to recover packet losses without retransmission round trips.
Each source frame is transmitted as a data packet containing a type field, sequence number, and 160-byte payload.
Frames are grouped in pairs, and the sender transmits one additional parity packet containing the XOR of both payloads.
The receiver stores data and parity packets indexed by sequence number.
If exactly one frame from a pair is missing and its parity packet arrives, the receiver reconstructs the missing payload using XOR.
Sequence numbers allow the receiver to handle packet reordering and ignore duplicate data packets.
This design uses approximately 1.55x bandwidth, remaining below the required 2.0x limit.
No feedback or retransmission is required, avoiding additional round-trip latency through the hostile network.
The recommended grading playout delay is 100 ms.
The design can fail under sufficiently severe burst loss when multiple data frames or the required parity packet are lost before their deadlines.
