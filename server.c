#include<stdio.h>
#include<stdlib.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<string.h>
#include<arpa/inet.h>
#include<fcntl.h> // for open
#include<unistd.h> // for close
#include<pthread.h>

#define NUM_OF_CARDS  72
#define MAX_PLAYERS  4

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

struct player_info{
  int num_cards_shown;
  int num_cards_hidden;
  struct Stack cards_shown;
  struct Queue cards_hidden;
};

struct game_info {
  int game_id;
  int numbers_of_players;
  struct player_info players[MAX_PLAYERS];
  int order;
  struct game_info *next;
  struct game_info *prvs;
  int player_socket[MAX_PLAYERS];
  int game_running;
};

struct List {
  struct game_info *top;
  struct game_info *bot;
}Game_list;

struct thread_info {
  int socket;
  struct game_info *gi;
  int player_id;
};

//structure functions
void push_q(struct Queue *kolejka,struct Card *karta);
struct Card *pull_q(struct Queue *kolejka);
void push_s(struct Stack *stos, struct Card *karta);
struct Card pull_s(struct Stack *stos);
void add_game(struct List *lista,struct game_info *gra);
void delete_game(struct List *lista,int id);

//game functions
void draw_a_card (struct game_info *gi, int who);
void raise_a_totem (struct game_info *gi,int player);
void give_cards (struct game_info *gi,int from , int to);
void take_all (struct game_info *gi, int who);
void send_game_state(struct game_info *gi);
void start_game(struct game_info *gi);

void * ListeningThread(void *arg){
  //gdy dolacza nowy gracz tworzymy mu nowy socket
  printf("New player entered \n");
  struct thread_info *ti =arg;
  struct game_info *gi = ti->gi;
  int newSocket = ti->socket;
  char player_messege[2];
  char exit_message[15] = "000000000000000";
  exit_message[0]=(char)99;
  printf("Finished listening setup %d\n",newSocket);
  for(;;){
    if(recv(newSocket , player_messege , 2 ,0)<1){
      printf("Receive failed \n");
      break;
    }
    if(gi->game_running==-1)break;
    if(gi->game_running==0)continue;
    //nasluchiwanie wiadomosci od gracza
    else printf ("odebrano \n");
    //if(player_messege[0]==NULL)continue;
    int code = (int)player_messege[0];
        if(code==1){
          draw_a_card(gi,ti->player_id);
        }
        if(code==2){
          raise_a_totem(gi,ti->player_id);
        }
        if(code==3){
          break;
        }
    memset(&player_messege, 0, sizeof (player_messege));
    }
    gi->game_running=-1;
    delete_game(&Game_list,gi->game_id);
    send(newSocket,exit_message,sizeof(exit_message),0);
    printf("Player exited the game\n");
    close(newSocket);
    pthread_exit(NULL);
}

int main(){
  //game variables
  int number_of_games=0;
  int success = 1;
  srand(time(NULL));
  //server variables
  int serverSocket, newSocket;
  struct sockaddr_in serverAddr;
  struct sockaddr_storage serverStorage;
  socklen_t addr_size;
  addr_size = sizeof serverStorage;
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

    while(1) //create a singular game here
    {
        struct game_info *gra = malloc(sizeof(struct game_info));
        gra->game_id=++number_of_games;
        gra->numbers_of_players=MAX_PLAYERS;
        gra->order=0;
        gra->game_running=0;
        int player_id = 0;
        add_game(&Game_list,gra);
        printf("Zalozono poczekalnie nr %d \n",gra->game_id);
        //printf("%d\n",Game_list.bot->game_id);
        //printf("%d\n",Game_list.top->game_id);
        //Accept call creates a new socket for the incoming connection
        while(player_id<MAX_PLAYERS)
          {
            if(listen(serverSocket,50)==0)
                printf("Listening\n");
            else
                printf("Error\n");    
            newSocket = accept(serverSocket, (struct sockaddr *) &serverStorage, &addr_size);
                printf("przyjeto gracza\n");
            //increasing the numbers of players
            gra->player_socket[player_id]=newSocket;
            struct thread_info *ti= malloc(sizeof(struct thread_info));
            ti->socket=newSocket;
            ti->gi=gra;
            ti->player_id=player_id++;   
            printf("Threading\n");
            pthread_t listen_id;
            if( (pthread_create(&listen_id, NULL, ListeningThread, ti) )!= 0 )
              printf("Failed to create thread\n");  
            printf("dodano gracza %d\n",ti->player_id);
          }
          sleep(2);
          printf("game is starting\n");
          start_game(gra);
    }
  return 0;
}

void push_q(struct Queue *kolejka,struct Card *karta){
  karta->below=NULL;
  if(kolejka->bot==NULL){
    printf("wrzucam pierwsza karte\n");
    kolejka->bot=karta;
    kolejka->top=karta;
  }
  else{
    kolejka->bot->below = karta;
    kolejka->bot=karta;
  }
}

struct Card *pull_q ( struct Queue *kolejka){
  struct Card *tpm=malloc(sizeof(struct Card));
  if(kolejka->top==NULL)return tpm;
  tpm = kolejka->top;
  kolejka->top = kolejka->top->below;
  return tpm;
}

void push_s(struct Stack *stos,struct Card *karta){
  karta->below=NULL;
  if(stos->top==NULL){
    printf("wrzucam pierwsza karte \n");
    stos->top=karta;
    karta->below=NULL;
  }
  else{
    karta->below=stos->top;
    stos->top=karta;
  }
}

struct Card pull_s ( struct Stack *stos){
  struct Card *output = malloc(sizeof(struct Card));
  if(stos->top==NULL) return *output;
  output = stos->top;
  stos->top = stos->top->below;
  return *output;
}

void add_game(struct List *lista ,struct game_info *gra){
  if(lista->top==NULL){
    printf("dodawanie pierwszej gry\n");
    lista->top=gra;
    lista->bot=gra;
    gra->next=NULL;
    gra->prvs=NULL;
  }
  else{
    lista->top->next=gra;
    gra->prvs=lista->top;
    gra->next=NULL;
    lista->top=gra;
  }
}

void delete_game(struct List *lista, int id){
  if(lista->bot==NULL)return;
  struct game_info *tmp= lista->bot;
  while(tmp->game_id!=id)
    {
      if(tmp->next==NULL)return;
      tmp=tmp->next;
    } 
  if(tmp==lista->bot && tmp==lista->top){
    lista->top=NULL;
    lista->bot=NULL;
  } 
  else if(tmp==lista->bot){
    lista->bot=lista->bot->next;
    lista->bot->prvs=NULL;
  }
  else if(tmp==lista->top){
    lista->top=lista->top->prvs;
    lista->top->next=NULL;
  }
  else{
  tmp->prvs->next=tmp->next;
  tmp->next->prvs=tmp->prvs;
  }
}

void give_cards (struct game_info *gi,int from , int to){
  printf("dracz %d daje karty graczowi %d \n",from ,to);
  for (int i=0 ; i<gi->players[from].num_cards_shown ; i++){
    struct Card *tmp = malloc(sizeof(struct Card));
    *tmp = pull_s( &gi->players[from].cards_shown);
    push_q(&gi->players[to].cards_hidden,tmp);
  }
  for (int i=0 ; i<gi->players[to].num_cards_shown ; i++){
    struct Card *tmp = malloc(sizeof(struct Card));
    *tmp = pull_s( &gi->players[to].cards_shown);
    push_q(&gi->players[to].cards_hidden,tmp);
  }
  gi->players[to].num_cards_hidden+=gi->players[from].num_cards_shown;
  gi->players[to].num_cards_hidden+=gi->players[to].num_cards_shown;
  gi->players[from].num_cards_shown = gi->players[to].num_cards_shown = 0;
  gi->order=to;  

  send_game_state(gi);
}

void take_all (struct game_info *gi, int who){
  printf("gracz %d bierze karty \n",who);
  int cards_added = 0;
  for (int i=0 ; i<gi->numbers_of_players ; i++){
    for ( int j=0; j<gi->players[i].num_cards_shown; j++){
      struct Card *tmp = malloc(sizeof(struct Card));
      *tmp = pull_s( &gi->players[i].cards_shown);
      push_q(&gi->players[who].cards_hidden,tmp);
    }
    cards_added+=gi->players[i].num_cards_shown;
    gi->players[i].num_cards_shown=0;   
  }
  gi->players[who].num_cards_hidden+=cards_added;
  gi->order=who;
  
  send_game_state(gi);
}

void draw_a_card (struct game_info *gi, int who){
  if(gi->players[who].num_cards_hidden==0){
    printf("Gracz %d nie ma kart \n",who);
    return;
  }

  if(gi->order!=who){
    printf("Gracz %d probowal sie ruszyc w kolejce gracza %d \n",who,gi->order);
    return;
  } 

  printf("Gracz %d dobiera karte \n",who);

  gi->players[who].num_cards_hidden--;
  gi->players[who].num_cards_shown++;
  struct Card *tmp = pull_q(&gi->players[who].cards_hidden);
  printf("wzialem %d\n",tmp->card_id);
  push_s(&gi->players[who].cards_shown,tmp);
  gi->order++;
  gi->order=gi->order%gi->numbers_of_players;
  while(gi->players[gi->order].num_cards_hidden==0){
    gi->order++;
    gi->order=gi->order%gi->numbers_of_players;
  }
  printf("dobrano karte \n");
  send_game_state(gi);

}

void raise_a_totem (struct game_info *gi,  int player){
  printf("Gracz %d podniosl totem \n",player);
  if(gi->players[player].num_cards_shown==0){
    printf("Gracz %d nie ma kart przed soba \n",player);
    return;
  }
  printf("gracz moze podniesc totem\n");
  int tmp=gi->players[player].cards_shown.top->card_id;
  for(int i=0; i<gi->numbers_of_players ; i++){
    if(i==player)continue;
    if(gi->players[i].num_cards_shown==0)continue;
    if(gi->players[i].cards_shown.top->card_id/4==tmp/4){
      give_cards(gi,player,i);
      return;  
    }
  }
  take_all(gi,player);
}

void send_game_state(struct game_info *gi){

  for(int i=0; i<gi->numbers_of_players;i++){
    if(gi->players->num_cards_hidden==0 && gi->players->num_cards_shown==0){
      printf("gracz %d wygral \n",i);
    }
  }

  for(int i=0; i<gi->numbers_of_players ; i++){
    //printf("tworze wiadomosc dla gracza %d\n",i);
    char message[]= "000000000000000000";
    message[0]=(char)110;
    message[1]=(char)110;
    message[2]=(char)110;
    message[3]=(char)gi->numbers_of_players;
    message[4]=(char)i;
    message[5]=(char)gi->order;
    //printf("flaga \n");
    for (int iter=0; iter<gi->numbers_of_players;iter++){
      message[3*iter+6]=(char)gi->players[iter].num_cards_hidden;
      message[3*iter+7]=(char)gi->players[iter].num_cards_shown;
      if(gi->players[iter].num_cards_shown==0) message[3*iter+8] = (char)75;
      else message[3*iter+8]=(char)gi->players[iter].cards_shown.top->card_id;
      //printf("flaga %d\n",iter);
    }
    printf("wysylam do %d \n",gi->player_socket[i]);
    send(gi->player_socket[i],message,sizeof(message),0);
  }
}

void start_game(struct game_info *gi){
 //zerowanie gry


  gi->game_running=1;
  for(int i=0; i<gi->numbers_of_players;i++)
  {
    gi->players[i].cards_shown.top=NULL;
    gi->players[i].cards_hidden.bot=NULL;
    gi->players[i].cards_hidden.top=NULL;
    gi->players[i].num_cards_hidden=0;
    gi->players[i].num_cards_shown=0;
  }
  printf("players setup done\n");
 //rozdanie kart

 int was_chosen[NUM_OF_CARDS];
 for(int i=0; i<NUM_OF_CARDS;i++) was_chosen[i]=0;
 int number_of_cards=NUM_OF_CARDS;
 int i=0;
  while((number_of_cards)>0){
      i++;
      i=i%MAX_PLAYERS;
      struct Card *tmp = malloc(sizeof(struct Card));
      tmp->below=NULL;
      int id=rand()%number_of_cards--;
      printf("wylosowano %d  ", id);
      int k=0;
      for(int j=0; j<id; k++){
          if(was_chosen[k]==0)j++;
      }
      while(was_chosen[k]==1)k++;
      tmp->card_id=k;

      was_chosen[k]=1;  
      printf("dano karte %d\n",k);
      push_q(&gi->players[i].cards_hidden,tmp);
      gi->players[i].num_cards_hidden++;
    }
  printf("Cards drawn\n");
  send_game_state(gi);   
  }
