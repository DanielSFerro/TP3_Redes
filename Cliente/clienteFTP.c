#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/time.h>
#include "tp_socket.c"


//CLIENTE
int main(int argc, char* argv[]){               //argv[1] = IP SERVIDOR
                                                //argv[2] = porta SERVIDOR
                                                //argv[3] = nome do ARQUIVO
                                                //argv[4] = TAMANHO DO BUFFER
ssize_t recvbytes;

char *nome_servidor = argv[1];
int porta_do_servidor = atoi(argv[2]);
char *nome_do_arquivo = argv[3];
int tam_buffer = atoi(argv[4]);


struct timeval a, b;

//char buffer[tam_buffer];

//char pacote[tam_buffer];
char* pacote = (char*)malloc(tam_buffer*sizeof(char*));         //os dois primeiros bits são ack/frame e verificacao
char* pacoteaux = (char*)malloc((tam_buffer-2)*sizeof(char*));  //pacoteaux so recebe os dados sem os dois primeiro bits
//define o bit frame do pacote como 0
pacote[0] = '0';

//dados a serem enviados
//char palavra[tam_buffer-1];


//checando número de argumentos:
if (argc != 5){
  fprintf(stderr, "use:./clienteFTP [IP] [Porta] [arquivo] [Tamanho]\n");
  return -1;
}

so_addr server;


//inicialização
tp_init();


//criando socket com porta do servidor
int sockfd;
sockfd = tp_socket(2000);

//conectando porta ao IP servidor:
if ((tp_build_addr(&server, nome_servidor, porta_do_servidor)) < 0){
  printf("ERRO");
  return -1;
};


//envia word para o servidor
//strncpy(palavra, nome_do_arquivo, sizeof(palavra));
//strcat(pacote, nome_do_arquivo);
memmove(pacote + 1, nome_do_arquivo, tam_buffer-1); // mesmo resultado obtido em strcat

tp_sendto(sockfd, pacote, tam_buffer, &server);

tp_recvfrom(sockfd, pacote, tam_buffer, &server);


printf("O nome do arquivo detectado pelo servidor é: %s\n", pacote);


////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

int tamanho_recebido;
int tamanho_do_cabecalho = 2;
char ack = '0';
char xor;
char aux;
int i;

FILE *received_file;
received_file = fopen(nome_do_arquivo, "w");
fflush(received_file);

do {
    tamanho_recebido = tp_recvfrom(sockfd, pacote, tam_buffer, &server);
    memmove(pacoteaux, pacote + 2, tam_buffer-2);
    xor = pacoteaux[0];
    for(i=1; i<tam_buffer-4; ++i)
        xor = (char)(xor ^ pacoteaux[i]);
    printf("Xor calculado é: %c\n", xor);
    //conferir se frame do sevidor está correto que por coincidência, é sempre igual a ack armazenado no código
    if ((pacote[0] == ack)){
      //SE O PACOTE NAO ESTA CORROMPIDO
      if (pacote[1] == xor){
        printf("Nao corrompido - pacote[1] é: %c\n", pacote[1]);
        printf("Nao corrompido - xor é: %c\n", xor);
        fflush(received_file);
        fwrite(pacoteaux, sizeof (char), tamanho_recebido-2, received_file);
        memset(pacote, 0, tam_buffer);
        memset(pacoteaux, 0, tam_buffer-1);
        //Lógica de inverter o ack, uma hora ele confere se é igual a '1', outra se é igual a '0'
        if (ack == '0'){
          ack = '1';
        }
        else{
          ack = '0';
        }

        //enviar ack de volta para o servidor
        pacote[0] = ack;
        pacote[1] = '0';
        tp_sendto(sockfd, pacote, tam_buffer, &server);
        memset(pacote, 0, tam_buffer);
      }
      //SE O PACOTE ESTA CORROMPIDO
      else {//pacote[1] != xor
          //não precisamos fazer fwrite
          //não precisamos inverter o ack neste caso
          printf("corrompido\n");
          printf("pacote[1] é: %c\n", pacote[1]);
          printf("xor é: %c\n", xor);
          pacote[1] = '1';
          tp_sendto(sockfd, pacote, tam_buffer, &server);
          memset(pacote, 0, tam_buffer);
      }
    }
    //caso o ack esteja errado, então já recebemos o dado mas o ack não voltou corretamente para o servidor,
    //então devemos descartar dados duplicados e enviar
    else{ //(pacote[0] != ack || pacote[1] == xor)
      //não precisamos fazer fwrite
      //não precisamos inverter o ack neste caso
      printf("errado e nao corrompido\n");
      pacote[0] = ack;
      pacote[1] = '0';
      tp_sendto(sockfd, pacote, tam_buffer, &server);
      memset(pacote, 0, tam_buffer);
    }

}while(tamanho_recebido != tamanho_do_cabecalho);

printf("FIM DO PROGRAMA");

fclose(received_file);
close(sockfd);
free(pacote);
free(pacoteaux);

return 0;
}
