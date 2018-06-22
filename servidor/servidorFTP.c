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

//Buffer a ser enviado:
char buffer[tam_buffer];
//Auxiliar do buffer - guarda o buffer antigo
//Se der erro, o buffer deve ser enviado novamente
//O valor antigo será enviado pelo auxbuffer
char auxbuffer[tam_buffer];

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
memset(buffer, 0, tam_buffer);

//Struct para o temporizador:
//Define o tempo de temporização como 1 segundo:
struct timeval tv;
  tv.tv_sec = 1; //segundos
  tv.tv_usec = 0; //microsegundos
  //Define tempo limite de operação do socket
setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv));
//manda o arquivo para o cliente


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
    if ((tp_recvfrom(socket, buffer, tam_buffer, &client)) == -1) {
      tp_sendto(socket, auxbuffer, tamanho_lido + 1, &client);
    }
    else{
        memset(auxbuffer, 0, tam_buffer);
        //conferir o ack recebido, que coincide com valor de frame. Caso recebeu, pode enviar o próximo dados.
        if (buffer[0] == frame){
          tamanho_lido = fread(auxbuffer, 1, tam_buffer-1, arquivo);
          buffer[0] = frame;
          memmove (buffer + 1, auxbuffer, tam_buffer-1);
          //auxbuffer recebe o valor de buffer, caso precise reenviar:
          strncpy (auxbuffer, buffer, tam_buffer);
          tp_sendto(socket, buffer, tamanho_lido + 1, &client);
          memset(buffer, 0, tam_buffer);
          if (frame == '0'){
            frame = '1';
          }
          else{
            frame = '0';
          }
        }
    }
  }

}while (tamanho_lido > 0);

//Fecha o socket:
close(socket);
//Fecha o arquivo:
fclose(arquivo);

return 0;
}
