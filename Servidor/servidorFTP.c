#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include "tp_socket.c"

int main(int argc, char* argv[]){        //argv[1] = PORTA
                                         //argv[2] = TAMANHO DO BUFFER

int porta_do_servidor = atoi(argv[1]);
int tam_buffer = atoi(argv[2]);

char buffer[tam_buffer];
char auxbuffer[tam_buffer];

//checando número de argumentos
if (argc != 3){
  fprintf(stderr, "use:./clienteFTP [Porta] [Tamanho]\n");
  return -1;
}

tp_init();

so_addr client;

int socket;

//criando socket servidor:
socket = tp_socket(porta_do_servidor);

if (tp_recvfrom(socket, buffer, tam_buffer, &client) <= 0){
  printf("erro");
  return -1;
}

//if ((tp_sendto(socket, buffer, tam_buffer, &client)) < 0){
//  printf("ERRO");
//}

printf("%s\n", buffer);

//recebe o buffer e tira o bit frame, deixando apenas o nome do arquivo em arquivo_requsiitado
char arquivo_requisitado[tam_buffer];
int i;
for(i=0; i<tam_buffer; i++){
    arquivo_requisitado[i] = buffer[i+1];
}

//manda o nome do arquivo de volta pro servidor
tp_sendto(socket, arquivo_requisitado, sizeof(arquivo_requisitado), &client);

printf("%s\n", arquivo_requisitado);

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

FILE *arquivo;
arquivo = fopen(arquivo_requisitado, "r");
if(arquivo == NULL){
  printf("ERRO AO ABRIR O ARQUIVO");
  return -1;
}


int tamanho_lido;
int conferir_primeira_leitura = 1;
char frame = '1';
char xor;
char verifica_xor_mesma_resposta;
memset(buffer, 0, tam_buffer);

//struct para o temporizador
struct timeval tv;
  tv.tv_sec = 1; //segundos
  tv.tv_usec = 0; //microsegundos
  //Define tempo limite de operação do socket
setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv));
//manda o arquivo para o cliente


do{
  //primeira leitura não precisa conferir ack correto
  if (conferir_primeira_leitura == 1){
    tamanho_lido = fread(auxbuffer, 1, tam_buffer-2, arquivo);

    char xor = auxbuffer[0];
    char verifica_xor_mesma_resposta = auxbuffer[0];

    for(i=1; i<tam_buffer-4; ++i)
      xor = (char)((char)xor ^ auxbuffer[i]);

    for(i=1; i<tam_buffer-4; ++i)
      verifica_xor_mesma_resposta = (char)(verifica_xor_mesma_resposta ^ auxbuffer[i]);

    printf("O valor de xor é: %c\n", xor);
    printf("O valor de verifica xor é: %c\n", verifica_xor_mesma_resposta);



    memmove (buffer + 2, auxbuffer, tam_buffer-2);
    buffer[0] = '0';
    buffer[1] = xor;
    tp_sendto(socket, buffer, tamanho_lido + 2, &client);
    printf("Primeira leitura - buffer[1] é: %c\n", buffer[1]);
    printf("Primeira leitura - xor é: %c\n", xor);
    strncpy (auxbuffer, buffer, tam_buffer);
    memset(buffer, 0, tam_buffer);
    conferir_primeira_leitura = -1;
  }
  //única diferença aqui é que agora confere também se o ack está correto
  else{
    if ((tp_recvfrom(socket, buffer, tam_buffer, &client)) == -1) {
      tp_sendto(socket, auxbuffer, tamanho_lido + 2, &client);
    }
    else{

      if (buffer[1] == '0') {
        memset(auxbuffer, 0, tam_buffer);
        //conferir o ack recebido, que coincide com valor de frame. Caso recebeu, pode enviar o próximo dados.
        if (buffer[0] == frame){
          tamanho_lido = fread(auxbuffer, 1, tam_buffer-2, arquivo);

          xor = auxbuffer[0];
          for(i=1; i<tam_buffer-4; ++i)
            xor = (char)(xor ^ auxbuffer[i]);

          buffer[0] = frame;
          buffer[1] = xor;
          memmove (buffer + 2, auxbuffer, tam_buffer-2);
          tp_sendto(socket, buffer, tamanho_lido + 2, &client);
          printf("buffer[1] é: %c\n", buffer[1]);
          strncpy (auxbuffer, buffer, tam_buffer);
          memset(buffer, 0, tam_buffer);
          if (frame == '0'){
            frame = '1';
          }
          else{
            frame = '0';
          }
        }
      }
      else{ //buffer[1] = '1' caso corrompido
        tp_sendto(socket, auxbuffer, tamanho_lido + 2, &client);
        printf("enviando de novo: parte corrompida:buffer[1] é: %c\n", auxbuffer[1]);
        printf("o xor é: %c\n", xor);
      }
    }
  }
  //não precisa conferir caso que não recebe o ack correto, mas depois tem que fazer a lógica do temporizador
  //de caso não recebeu o ack

}while (tamanho_lido > 0);

close(socket);
fclose(arquivo);

return 0;
}
