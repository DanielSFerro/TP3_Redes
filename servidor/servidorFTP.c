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
                                         //argv[3] = TAMANHO DA JANELA

int porta_do_servidor = atoi(argv[1]);
int tam_buffer = atoi(argv[2]);
int tam_janela = atoi(argv[3]);


//Buffer a ser enviado:
char buffer[tam_buffer];
//Auxiliar do buffer - guarda o buffer antigo
//Se der erro, o buffer deve ser enviado novamente
//O valor antigo será enviado pelo auxbuffer
char auxbuffer[tam_buffer];

char buffer_matriz[tam_janela][tam_buffer];
char auxbuffer_matriz[tam_janela][tam_buffer];
// char auxbuffer0[tam_buffer];
// char auxbuffer1[tam_buffer];
// char auxbuffer2[tam_buffer];
// char auxbuffer3[tam_buffer];
// char auxbuffer4[tam_buffer];
// char auxbuffer5[tam_buffer];
// char auxbuffer6[tam_buffer];
// char auxbuffer7[tam_buffer];
// char auxbuffer8[tam_buffer];
// char auxbuffer9[tam_buffer];



//Checando número de argumentos:
if (argc != 3){
  fprintf(stderr, "use:./clienteFTP [Porta] [Tamanho]\n");
  return -1;
}

tp_init();

so_addr client;

int socket;

//criando socket servidor:
socket = tp_socket(porta_do_servidor);

//recebe o nome do arquivo:
if (tp_recvfrom(socket, buffer, tam_buffer, &client) <= 0){
  printf("\nErro.\n");
  return -1;
}

//Imprime o nome do arquivo + bit frame
printf("%s\n", buffer);

//recebe o buffer e tira o bit frame, deixando apenas o nome do arquivo em arquivo_requsiitado
char arquivo_requisitado[tam_buffer];
int i;
for(i=0; i<tam_buffer; i++){
    arquivo_requisitado[i] = buffer[i+1];
}

//manda o nome do arquivo de volta pro servidor, sem bit frame:
tp_sendto(socket, arquivo_requisitado, sizeof(arquivo_requisitado), &client);

printf("%s\n", arquivo_requisitado);

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////
////////////////////ENVIA DADOS AO CLIENTE//////////////////////
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

int ack_int = 0;
char ack_char = '0';
memset(buffer, 0, tam_buffer);
int aux_i;

//Struct para o temporizador:
//Define o tempo de temporização como 1 segundo:
struct timeval tv;
  tv.tv_sec = 1; //segundos
  tv.tv_usec = 0; //microsegundos
  //Define tempo limite de operação do socket
setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv));
//manda o arquivo para o cliente

int j;

do{
  //primeira leitura não precisa conferir ack correto:
  if (conferir_primeira_leitura == 1){
    tamanho_lido = fread(auxbuffer, 1, tam_buffer-1, arquivo);
    buffer[0] = '0';
    memmove (buffer + 1, auxbuffer, tam_buffer-1);
    tp_sendto(socket, buffer, tamanho_lido + 1, &client);
    strncpy (auxbuffer, buffer, tam_buffer);
    memset(buffer, 0, tam_buffer);
    //depois da primeira leitura esse if não será acessado novamente:
    conferir_primeira_leitura = -1;
  }
  //única diferença aqui é que agora confere também se o ack está correto
  else{
    //se o ACK estiver errado, reenvia os dados com o buffer anterior (auxbuffer)
    //                          reenvia os aux_buffer
    for(i=0; i<=tam_janela; i++){
      ack_int = i;

      if ((tp_recvfrom(socket, buffer, tam_buffer, &client)) == -1) {

      for(j = ack_int; j<tam_janela; j++)
          tp_sendto(socket, auxbuffer_matriz[j], tamanho_lido + 1, &client);
      }

      else if (buffer[0] != frame){
        for(j = aux_i+1; j<tam_janela; j++)
            tp_sendto(socket, auxbuffer_matriz[j], tamanho_lido + 1, &client);
      }
      else{
        aux_i = atoi(buffer[0]);
      }
      //////////////////////////////////////////////////////////////////////
      ///////////////////////////////////////////////////////////////////////
    }

    int frame_num = 0;
    for(i=0; i<tam_janela; i++){
        memset(auxbuffer, 0, tam_buffer);
        //conferir o ack recebido, que coincide com valor de frame. Caso recebeu, pode enviar o próximo dados.
        if (frame = '9'){
          frame_num = 0;
          frame = '0';
        }
        else{
          frame_num = frame_num + 1;
          itoa(frame_num, frame, 10);
        }

          tamanho_lido = fread(auxbuffer_matriz[i], 1, tam_buffer-1, arquivo);
          buffer_matriz[i][0] = frame;
          memmove (buffer_matriz[i][1], auxbuffer_matriz[i][tam_buffer], tam_buffer-1);
          //auxbuffer recebe o valor de buffer, caso precise reenviar:
          strncpy (auxbuffer_matriz[i], buffer_matriz[i], tam_buffer);
          tp_sendto(socket, buffer_matriz[i], tamanho_lido + 1, &client);
          memset(buffer, 0, tam_buffer);
        }
      }

}while (tamanho_lido > 0);

//Fecha o socket:
close(socket);
//Fecha o arquivo:
fclose(arquivo);

return 0;
}
