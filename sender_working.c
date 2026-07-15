#include <arpa/inet.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define FRAME_SIZE 164
#define PAYLOAD_SIZE 160
#define GROUP_SIZE 2

/*
Wire format:

Data packet:
byte 0      : type = 0
bytes 1-4   : original 4-byte sequence number
bytes 5-164 : 160-byte payload

Parity packet:
byte 0      : type = 1
bytes 1-4   : sequence number of first frame in group
bytes 5-164 : XOR of the four 160-byte payloads
*/

int main(void) {
    int in_fd = socket(AF_INET, SOCK_DGRAM, 0);

    struct sockaddr_in in_addr = {0};
    in_addr.sin_family = AF_INET;
    in_addr.sin_port = htons(47010);
    in_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (bind(in_fd, (struct sockaddr *)&in_addr, sizeof in_addr) < 0) {
        perror("bind 47010");
        return 1;
    }

    int out_fd = socket(AF_INET, SOCK_DGRAM, 0);

    struct sockaddr_in relay = {0};
    relay.sin_family = AF_INET;
    relay.sin_port = htons(47001);
    relay.sin_addr.s_addr = inet_addr("127.0.0.1");

    unsigned char input[FRAME_SIZE];
    unsigned char packet[165];

    unsigned char parity[PAYLOAD_SIZE];
    memset(parity, 0, sizeof(parity));

    uint32_t group_start = 0;
    int group_count = 0;

    for (;;) {
        ssize_t n = recvfrom(
            in_fd,
            input,
            sizeof(input),
            0,
            NULL,
            NULL
        );

        if (n != FRAME_SIZE)
            continue;

        uint32_t seq_net;
        memcpy(&seq_net, input, 4);

        uint32_t seq = ntohl(seq_net);

        /*
         * Send original data packet.
         */
        packet[0] = 0;

        memcpy(packet + 1, input, FRAME_SIZE);

        sendto(
            out_fd,
            packet,
            sizeof(packet),
            0,
            (struct sockaddr *)&relay,
            sizeof(relay)
        );

        /*
         * Start a new FEC group.
         */
        if (group_count == 0) {
            group_start = seq;
            memset(parity, 0, sizeof(parity));
        }

        /*
         * XOR payload into parity.
         */
        for (int i = 0; i < PAYLOAD_SIZE; i++) {
            parity[i] ^= input[4 + i];
        }

        group_count++;

        /*
         * After 4 frames, send one parity packet.
         */
        if (group_count == GROUP_SIZE) {
            unsigned char parity_packet[165];

            parity_packet[0] = 1;

            uint32_t start_net = htonl(group_start);

            memcpy(parity_packet + 1, &start_net, 4);
            memcpy(parity_packet + 5, parity, PAYLOAD_SIZE);

            sendto(
                out_fd,
                parity_packet,
                sizeof(parity_packet),
                0,
                (struct sockaddr *)&relay,
                sizeof(relay)
            );

            group_count = 0;
        }
    }

    return 0;
}
