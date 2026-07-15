#include <arpa/inet.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define PAYLOAD_SIZE 160
#define GROUP_SIZE 2
#define MAX_FRAMES 10000

static unsigned char frames[MAX_FRAMES][PAYLOAD_SIZE];
static unsigned char received[MAX_FRAMES];

static unsigned char parity_data[MAX_FRAMES][PAYLOAD_SIZE];
static unsigned char parity_received[MAX_FRAMES];

static void send_frame(
    int out_fd,
    struct sockaddr_in *player,
    uint32_t seq,
    unsigned char *payload
) {
    unsigned char output[164];

    uint32_t seq_net = htonl(seq);

    memcpy(output, &seq_net, 4);
    memcpy(output + 4, payload, PAYLOAD_SIZE);

    sendto(
        out_fd,
        output,
        sizeof(output),
        0,
        (struct sockaddr *)player,
        sizeof(*player)
    );
}

static void try_recover(
    int out_fd,
    struct sockaddr_in *player,
    uint32_t group_start
) {
    if (group_start >= MAX_FRAMES)
        return;

    if (!parity_received[group_start])
        return;

    int missing_count = 0;
    uint32_t missing_seq = 0;

    for (int i = 0; i < GROUP_SIZE; i++) {
        uint32_t seq = group_start + i;

        if (seq >= MAX_FRAMES)
            return;

        if (!received[seq]) {
            missing_count++;
            missing_seq = seq;
        }
    }

    /*
     * XOR parity can recover exactly one missing frame.
     */
    if (missing_count != 1)
        return;

    unsigned char recovered[PAYLOAD_SIZE];

    memcpy(
        recovered,
        parity_data[group_start],
        PAYLOAD_SIZE
    );

    for (int i = 0; i < GROUP_SIZE; i++) {
        uint32_t seq = group_start + i;

        if (seq == missing_seq)
            continue;

        for (int j = 0; j < PAYLOAD_SIZE; j++) {
            recovered[j] ^= frames[seq][j];
        }
    }

    memcpy(
        frames[missing_seq],
        recovered,
        PAYLOAD_SIZE
    );

    received[missing_seq] = 1;

    /*
     * Immediately send reconstructed frame to player.
     */
    send_frame(
        out_fd,
        player,
        missing_seq,
        recovered
    );
}

int main(void) {
    int in_fd = socket(AF_INET, SOCK_DGRAM, 0);

    struct sockaddr_in in_addr = {0};
    in_addr.sin_family = AF_INET;
    in_addr.sin_port = htons(47002);
    in_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (bind(
        in_fd,
        (struct sockaddr *)&in_addr,
        sizeof(in_addr)
    ) < 0) {
        perror("bind 47002");
        return 1;
    }

    int out_fd = socket(AF_INET, SOCK_DGRAM, 0);

    struct sockaddr_in player = {0};
    player.sin_family = AF_INET;
    player.sin_port = htons(47020);
    player.sin_addr.s_addr = inet_addr("127.0.0.1");

    unsigned char packet[2048];

    for (;;) {
        ssize_t n = recvfrom(
            in_fd,
            packet,
            sizeof(packet),
            0,
            NULL,
            NULL
        );

        if (n != 165)
            continue;

        unsigned char type = packet[0];

        uint32_t seq_net;
        memcpy(&seq_net, packet + 1, 4);

        uint32_t seq = ntohl(seq_net);

        /*
         * DATA PACKET
         */
        if (type == 0) {
            if (seq >= MAX_FRAMES)
                continue;

            /*
             * Ignore network duplicates.
             */
            if (received[seq])
                continue;

            memcpy(
                frames[seq],
                packet + 5,
                PAYLOAD_SIZE
            );

            received[seq] = 1;

            /*
             * Send original frame immediately.
             */
            send_frame(
                out_fd,
                &player,
                seq,
                frames[seq]
            );

            /*
             * Find this frame's FEC group.
             */
            uint32_t group_start =
                (seq / GROUP_SIZE) * GROUP_SIZE;

            try_recover(
                out_fd,
                &player,
                group_start
            );
        }

        /*
         * PARITY PACKET
         */
        else if (type == 1) {
            uint32_t group_start = seq;

            if (group_start >= MAX_FRAMES)
                continue;

            memcpy(
                parity_data[group_start],
                packet + 5,
                PAYLOAD_SIZE
            );

            parity_received[group_start] = 1;

            try_recover(
                out_fd,
                &player,
                group_start
            );
        }
    }

    return 0;
}
