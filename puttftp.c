//
// Created by carpe-bleue on 11/12/24.
//

#include "puttftp.h"

// Fonction pour envoyer une requête WRQ
void send_wrq(int sockfd, struct sockaddr_in *server_addr, const char *filename) {
    char buffer[MAX_BUF_SIZE];
    int len;

    // Construire la requête WRQ : [filename] + [mode]
    len = snprintf(buffer, MAX_BUF_SIZE, "%s%c%s%c", filename, 0, "octet", 0);

    // Envoyer la requête WRQ au serveur
    sendto(sockfd, buffer, len, 0, (struct sockaddr *)server_addr, sizeof(*server_addr));
}

// Fonction principale de puttftp
int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <server> <filename>\n", argv[0]);
        exit(1);
    }

    int sockfd;
    struct sockaddr_in server_addr;
    char buffer[MAX_BUF_SIZE];
    FILE *file;
    int bytes_sent;
    int block_num = 1;

    // Création du socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Socket creation failed");
        exit(1);
    }

    // Remplir la structure sockaddr_in avec les informations du serveur
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(TFTP_PORT);
    server_addr.sin_addr.s_addr = inet_addr(argv[1]);

    // Envoyer la requête WRQ
    send_wrq(sockfd, &server_addr, argv[2]);

    // Ouvrir le fichier local pour lecture
    file = fopen(argv[2], "rb");
    if (file == NULL) {
        perror("File opening failed");
        close(sockfd);
        exit(1);
    }

    // Envoyer le fichier en paquets de données
    while ((bytes_sent = fread(buffer + 4, 1, MAX_BUF_SIZE - 4, file)) > 0) {
        buffer[0] = 0;
        buffer[1] = 3; // Type de paquet DAT
        buffer[2] = (block_num >> 8) & 0xFF;  // Numéro de bloc (MSB)
        buffer[3] = block_num & 0xFF;         // Numéro de bloc (LSB)

        sendto(sockfd, buffer, bytes_sent + 4, 0, (struct sockaddr *)&server_addr, sizeof(server_addr));

        // Attendre l'ACK
        recvfrom(sockfd, buffer, 4, 0, NULL, NULL); // Attente de l'ACK

        block_num++;
    }

    // Fermer le fichier et le socket
    fclose(file);
    close(sockfd);
    return 0;
}
