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
                                                //argv[5] = TAMANHO DA JANELA
ssize_t recvbytes;

char *nome_servidor = argv[1];
int porta_do_servidor = atoi(argv[2]);
char *nome_do_arquivo = argv[3];
int tam_buffer = atoi(argv[4]);
int tam_janela = atoi(argv[5]);

//Pacote:
char* pacote = (char*)malloc(tam_buffer * sizeof(char*));
//Pacote auxiliar, só os dados(não tem o bit 0):
char* pacoteaux = (char*)malloc((tam_buffer-1) * sizeof(char*));
//Define o bit frame do pacote como 0:
pacote[0] = '0';

//checando número de argumentos:
if (argc != 6){
  fprintf(stderr, "use:./clienteFTP [IP] [Porta] [arquivo] [Tamanho] [JANELA]\n");
  return -1;
}

so_addr server;


//inicialização
tp_init();

//estrutura para análise de tempo:
struct timeval start, end;
gettimeofday(&start, NULL);

//criando socket com porta do servidor
int sockfd;
sockfd = tp_socket(2000);

//conectando porta ao IP servidor:
if ((tp_build_addr(&server, nome_servidor, porta_do_servidor)) < 0){
  printf("ERRO");
  return -1;
};


//Primeiro pacote enviado para o cliente: nome do arquivo
//Pacote[1] em diante recebe dados; pacote[0] é o bit frame já definido
memmove(pacote + 1, nome_do_arquivo, sizeof(nome_do_arquivo)+1);

//Envia nome do arquivo para o servidor + bit frame:
tp_sendto(sockfd, pacote, tam_buffer, &server);

//Recebe o nome do arquivo sem o bit frame
//(testa se o servidor consegue separar bit frame de dados)
tp_recvfrom(sockfd, pacote, tam_buffer, &server);

printf("\nO nome do arquivo detectado pelo servidor é: %s\n", pacote);


////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////
////////////////////RECEBE DADOS DO SERVIDOR////////////////////
////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

int tamanho_recebido; //recebe tamanho do buffer pela função tp_recvfrom
int tamanho_do_cabecalho = 1;
char ack = '0';
char aux;
int ack_num;
int i;
int Tamanho_Arquivo = 0;
FILE *received_file;
received_file = fopen(nome_do_arquivo, "w");
fflush(received_file);

do {
    tamanho_recebido = tp_recvfrom(sockfd, pacote, tam_buffer, &server);
    Tamanho_Arquivo = Tamanho_Arquivo + tamanho_recebido;
    //conferir se frame do sevidor está correto que por coincidência, é sempre igual a ack armazenado no código
    if ((pacote[0] == ack)){
      memmove(pacoteaux, pacote + 1, tam_buffer-1);
      fflush(received_file);
      fwrite(pacoteaux, sizeof (char), tamanho_recebido-1, received_file);
      memset(pacote, 0, tam_buffer);
      memset(pacoteaux, 0, tam_buffer-1);
      //Lógica de inverter o ack, uma hora ele confere se é igual a '1', outra se é igual a '0'
      if (ack_num = tam_janela){
        ack_num = 0;
        ack = '0';
      }
      else{
        ack_num = ack_num + 1;
        itoa (ack_num,ack,10);
      }

      //enviar ack de volta para o servidor
      pacote[0] = ack;
      tp_sendto(sockfd, pacote, tam_buffer, &server);
      memset(pacote, 0, tam_buffer);
    }

    //caso o ack esteja diferente de pacote[0],
    //então o dado já foi recebido mas o ack não voltou corretamente para o servidor,
    //então deve-se enviar o ack novamente
    else{ //(pacote[0] != ack)
      pacote[0] = ack;
      tp_sendto(sockfd, pacote, tam_buffer, &server);
      memset(pacote, 0, tam_buffer);
      //se for diferente de ack, o servidor enviara o buffer de nome_do_arquivo
      //dessa forma, Tamanho_Arquivo sera incrementado duas vezes
      //Tamanho_Arquivo deve ser decrementado se o arquivo for re-enviado
      Tamanho_Arquivo = Tamanho_Arquivo - tamanho_recebido;
    }

}while(tamanho_recebido != tamanho_do_cabecalho);

//fecha o arquivo:
fclose(received_file);
//fecha o socket:
close(sockfd);

//libera memória alocada com malloc:
free(pacote);
free(pacoteaux);

//Cálculo do tempo de transmissão:
gettimeofday(&end, NULL);

//calcula o tempo gasto em microssegundos
long int Tempo_micro = (end.tv_sec * 1000000 + end.tv_usec) - (start.tv_sec * 1000000 + start.tv_usec);
float Tempo_mili = Tempo_micro/1000;
float taxa_transmissao = Tamanho_Arquivo/Tempo_mili;
printf("\n%i\n", Tempo_micro);
printf("\nTamanho do buffer: %i\n", tam_buffer);
printf("\nTamanho_Arquivo: %d bytes\nTempo de transmissão:%ld microssegundos\nTaxa de transmissão: %f bytes/ms", Tamanho_Arquivo, Tempo_micro, taxa_transmissao);

printf("\nFIM DO PROGRAMA\n");

return 0;
}
