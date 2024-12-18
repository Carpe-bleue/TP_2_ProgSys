//
// Created by carpe-bleue on 11/12/24.
//

#include "gettftp.h"

// Fonction pour envoyer une requête RRQ
void send_rrq(int sockfd, struct sockaddr_in *server_addr, const char *filename) {
    char buffer[MAX_BUF_SIZE];
    int len;

    // Construire requête RRQ : [filename] + [mode]
    len = snprintf(buffer, MAX_BUF_SIZE, "%s%c%s%c", filename, 0, "octet", 0);

    // Envoyer requête RRQ au serveur
    sendto(sockfd, buffer, len, 0, (struct sockaddr *)server_addr, sizeof(*server_addr));
}


int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <server> <filename>\n", argv[0]);
        exit(1);
    }

    int sockfd;            //creation socket
    struct sockaddr_in server_addr;
    char buffer[MAX_BUF_SIZE];
    FILE *file;
    int bytes_received;

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

    // Envoyer la requête RRQ
    send_rrq(sockfd, &server_addr, argv[2]);

    // Ouvrir le fichier local pour écrire les données
    file = fopen(argv[2], "wb");
    if (file == NULL) {
        perror("File opening failed");
        close(sockfd);
        exit(1);
    }

    // Recevoir les paquets de données et les écrire dans le fichier
    while ((bytes_received = recvfrom(sockfd, buffer, MAX_BUF_SIZE, 0, NULL, NULL)) > 0) {
        fwrite(buffer + 4, 1, bytes_received - 4, file); // Ignorer le header de 4 octets (block # + opcode)

        // Envoi de l'ACK pour confirmer la réception
        char ack[4] = {0};
        ack[0] = 0; ack[1] = 4; // Type d'ACK
        ack[2] = buffer[2]; ack[3] = buffer[3]; // Block number
        sendto(sockfd, ack, 4, 0, (struct sockaddr *)&server_addr, sizeof(server_addr));

        // Vérifier si c'est le dernier paquet
        if (bytes_received < MAX_BUF_SIZE) {
            break; // Fin du transfert
        }
    }

    // Fermer le fichier et le socket
    fclose(file);
    close(sockfd);
    return 0;
}
