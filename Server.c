#include<stdio.h>
#include<stdlib.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<string.h>
#include<arpa/inet.h>
#include<fcntl.h> // for open
#include<unistd.h> // for close
#include<pthread.h>

struct Card {
  int card_id;
  struct Card *below;
};

struct Queue {
  struct Card *top;
  struct Card *bot;
};

struct Stack {
  struct Card *top;
};


void push_q(struct Queue kolejka,struct Card karta);
struct Card pull_q(struct Queue kolejka);
void push_s(struct Stack stos, struct Card karta);
struct Card pull_s(struct Stack stos);

struct player_info{
  int num_cards_shown;
  int num_cards_hidden;
  struct Stack cards_shown;
  struct Queue cards_hidden;
};

struct game_info {
  int numbers_of_players;
  struct player_info players[4];
  int order;
}gi;


void draw_a_card ( int who);
void raise_a_totem (int player);
void give_cards (int from , int to);
void take_all ( int who);
void send_game_state();
void start_game();

int player_messege[2];
int buffer[15];
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
int should_send[4];
int player_joined[4];

void * TalkingThread(void *arg , int player_id){
  printf("New player talking \n");
  int newSocket = *((int *)arg);
  int n;
  for(;;){
    while(should_send[player_id]==0)continue;
    should_send[player_id]=0;
    char *message = malloc(15);
    strcpy(message,(char)gi.numbers_of_players);
    strcpy(message,(char)player_id);
    strcpy(message,(char)gi.order);
    strcpy(message,(char)gi.players[0].num_cards_hidden);
    strcpy(message,(char)gi.players[0].num_cards_shown);
    strcpy(message,(char)gi.players[0].cards_shown.top->card_id);
    strcpy(message,(char)gi.players[1].num_cards_hidden);
    strcpy(message,(char)gi.players[1].num_cards_shown);
    strcpy(message,(char)gi.players[1].cards_shown.top->card_id);
    strcpy(message,(char)gi.players[2].num_cards_hidden);
    strcpy(message,(char)gi.players[2].num_cards_shown);
    strcpy(message,(char)gi.players[2].cards_shown.top->card_id);
    strcpy(message,(char)gi.players[3].num_cards_hidden);
    strcpy(message,(char)gi.players[3].num_cards_shown);
    strcpy(message,(char)gi.players[3].cards_shown.top->card_id);
    send(newSocket,message,sizeof(message),0);
  }
    player_joined[player_id]=0;
    printf("Player stopped talking\n");
    pthread_exit(NULL);
}

void * ListeningThread(void *arg ){
  //gdy dolacza nowy gracz tworzymy mu nowy socket
  printf("New player entered \n");
  int newSocket = *((int *)arg);
  int n;
  for(;;){
    //nasluchiwanie wiadomosci od gracza
    n=recv(newSocket , player_messege , 2 , 0);
    printf("%s\n",player_messege);
        if(n<1){
            break;
        }
        if((int)player_messege[0]==0){
          start_game();
        }
        if((int)player_messege[0]==1){
          draw_a_card(  (int)player_messege[1]);
        }
        if((int)player_messege[0]==2){
          raise_a_totem((int)player_messege[1]);
        }
    memset(&player_messege, 0, sizeof (player_messege));
    }
    printf("Player exited the game\n");
    pthread_exit(NULL);
}

int main(){
  //game setup
  int player_id = 0;
  should_send[0]=should_send[1]=should_send[2]=should_send[3]=0;
  player_joined[0]=player_joined[1]=player_joined[2]=player_joined[3]=0; 
  gi.numbers_of_players=0;
  gi.order=0;

  //server variables
  int serverSocket, newSocket;
  struct sockaddr_in serverAddr;
  struct sockaddr_storage serverStorage;
  socklen_t addr_size;

  //Create the socket. 
  serverSocket = socket(PF_INET, SOCK_STREAM, 0);

  // Configure settings of the server address struct
  // Address family = Internet 
  serverAddr.sin_family = AF_INET;

  //Set port number, using htons function to use proper byte order 
  serverAddr.sin_port = htons(1100);

  //Set IP address to localhost 
  serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);


  //Set all bits of the padding field to 0 
  memset(serverAddr.sin_zero, '\0', sizeof serverAddr.sin_zero);

  //Bind the address struct to the socket 
  bind(serverSocket, (struct sockaddr *) &serverAddr, sizeof(serverAddr));

  //Listen on the socket
  if(listen(serverSocket,50)==0)
    printf("Listening\n");
  else
    printf("Error\n");
    pthread_t listen_id;
    pthread_t talk_id;

    while(1)
    {
        //Accept call creates a new socket for the incoming connection
        addr_size = sizeof serverStorage;
        newSocket = accept(serverSocket, (struct sockaddr *) &serverStorage, &addr_size);

        //blocking an accept if there's too many players and assigning the first free id to him
        player_id=-1;
        for (int i=0; i<4;i++)
          if(player_joined[i]==1)
            continue;
          else player_id=i;
        if(player_id==-1)continue;
        //increasing the numbers of players 

        if( pthread_create(&listen_id, NULL, ListeningThread, &newSocket) != 0 )
           printf("Failed to create thread\n");
        if( pthread_create(&talk_id, NULL, TalkingThread, (&newSocket, player_id)) != 0 )
           printf("Failed to create thread\n");   

        pthread_detach(listen_id);
        pthread_detach(talk_id);
        //pthread_join(thread_id,NULL);
    }
  return 0;
}

void push_q(struct Queue kolejka,struct Card karta){
  kolejka.bot->below = &karta;
  kolejka.bot=&karta;
}

struct Card pull_q ( struct Queue kolejka){
  struct Card output = *kolejka.top;
  kolejka.top = kolejka.top->below;
  return output;
}

void push_s(struct Stack stos,struct Card karta){
  karta.below=stos.top;
  stos.top=&karta;
}

struct Card pull_s ( struct Stack stos){
  struct Card output = *stos.top;
  stos.top = stos.top->below;
  return output;
}

void give_cards (int from , int to){
  for (int i=0 ; i<gi.players[from].num_cards_shown ; i++){
    struct Card tmp =pull_s( gi.players[from].cards_shown);
    push_q(gi.players[to].cards_hidden,tmp);
  }
  for (int i=0 ; i<gi.players[to].num_cards_shown ; i++){
    struct Card tmp =pull_s( gi.players[to].cards_shown);
    push_q(gi.players[to].cards_hidden,tmp);
  }
  gi.players[to].num_cards_hidden+=gi.players[from].num_cards_shown;
  gi.players[to].num_cards_hidden+=gi.players[to].num_cards_shown;
  gi.players[from].num_cards_shown = gi.players[to].num_cards_shown = 0;
  gi.order=to;  

  send_game_state();
}


void take_all ( int who){
  int cards_added = 0;
  for (int i=1 ; i<gi.numbers_of_players ; i++){
    for ( int j=0; j<gi.players[i].num_cards_shown ; j++){
      struct Card tmp =pull_s( gi.players[i].cards_shown);
      push_q(gi.players[who].cards_hidden,tmp);
    }
    cards_added+=gi.players[i].num_cards_shown;
    gi.players[i].num_cards_shown=0;   
  }
  gi.players[who].num_cards_hidden+=cards_added;
  gi.order=who;

  send_game_state();
}

void draw_a_card ( int who){
  if(gi.order!=who) return;

  gi.players[who].num_cards_hidden--;
  gi.players[who].num_cards_shown++;
  struct Card tmp = pull_q(gi.players[who].cards_hidden);
  push_s(gi.players[who].cards_shown,tmp);
  gi.order++;
  gi.order=gi.order%gi.numbers_of_players;

  send_game_state();
}

void raise_a_totem (int player){
  int tmp=gi.players[player].cards_shown.top->card_id;
  for(int i=0; i<gi.numbers_of_players ; i++){
    if(i==player)continue;
    if(gi.players[i].cards_shown.top->card_id==tmp){
      give_cards(player,i);
      return;  
    }
  }
  take_all(player);
}

void send_game_state(){
  for(int i=0; i<gi.numbers_of_players ; i++)
    should_send[i]=1;
}

void start_game(){

  //wyzerowanie gry
  gi.order=0;
  gi.numbers_of_players=0;
  for (int i=0; i<4;i++){
    if(player_joined[i]==1)gi.numbers_of_players++;
    gi.players[i].num_cards_shown=0;
    gi.players[i].num_cards_hidden=0;
    gi.players[i].cards_hidden.top=NULL;
    gi.players[i].cards_hidden.bot=NULL;
    gi.players[i].cards_shown.top=NULL;
  }
 
 //rozdanie kart
 int was_chosen[80]={0};
 int number_of_cards=80;
 int i=0;
  while(number_of_cards--){
      i++;
      i=i%gi.numbers_of_players;
      struct Card tmp;
      tmp.below=NULL;
      int id=rand()%number_of_cards;
      while(was_chosen[id]==1)
        id=rand()%number_of_cards;
      tmp.card_id=id;
      was_chosen[id]=1;  
      push_q(gi.players[i].cards_hidden,tmp);
      gi.players[i].num_cards_hidden++;
    } 
  }