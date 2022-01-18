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
struct game_info find_a_game (struct List lista,int id);

//game functions
void draw_a_card (struct game_info *gi, int who);
void raise_a_totem (struct game_info *gi,int player);
void give_cards (struct game_info *gi,int from , int to);
void take_all (struct game_info *gi, int who);
void send_game_state(struct game_info *gi);
void start_game(struct game_info *gi);

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

/*void * TalkingThread(void *arg){
  printf("New player talking \n");
  struct thread_info *ti =arg;
  struct game_info *gi =ti->gi;
  int newSocket = ti->socket;
  int player_id = ti->player_id;
  int n=gi->should_send[player_id];
  printf(" pid czytania %d\n",getpid());
  printf("Finished talking setup\n");
  for(;;){
    printf("Am talking to  %d %d \n",gi,ti->player_id);
    while(n!=1){
      printf("flaga %d to %d \n",&gi->should_send[player_id] ,gi->should_send[player_id]);
      sleep(5);
      n=gi->should_send[player_id];
      }
    printf("Am talking to  %d %d \n",gi,ti->player_id);
    gi->should_send[player_id]=0;
    char message[3+3*gi->numbers_of_players];
    message[0]=(char)gi->numbers_of_players;
    message[1]=(char)player_id;
    message[2]=(char)gi->order;
    for (int iter=0; iter<gi->numbers_of_players;iter++){
      message[3*iter+3]=(char)gi->players[iter].num_cards_hidden;
      message[3*iter+4]=(char)gi->players[iter].num_cards_shown;
      message[3*iter+5]=(char)gi->players[iter].cards_shown.top->card_id;
    }
    send(newSocket,message,sizeof(message),0);
    sleep(100);
  }
    delete_game(&Game_list,gi->game_id);
    printf("Player stopped talking\n");
    pthread_exit(NULL);
}*/

void * socketThread(void *arg)
{
  char client_message[2000];
  printf("new thread \n");
  int newSocket = *((int *)arg);
  int n;
  for(;;){
    n=recv(newSocket , client_message , 2000 , 0);
    printf("%s\n",client_message);
        if(n<1){
            break;
        }

    char *message = malloc(sizeof(client_message));
    strcpy(message,client_message);

    sleep(1);
    send(newSocket,message,sizeof(message),0);
    memset(&client_message, 0, sizeof (client_message));

    }
    printf("Exit socketThread \n");

    pthread_exit(NULL);
}


void * ListeningThread(void *arg){
  //gdy dolacza nowy gracz tworzymy mu nowy socket
  printf("New player entered \n");
  struct thread_info *ti =arg;
  struct game_info *gi = ti->gi;
  int newSocket = ti->socket;
  char player_messege[2];
  printf("Finished listening setup %d\n",newSocket);
  for(;;){
    //nasluchiwanie wiadomosci od gracza
    if(recv(newSocket , player_messege , 2 ,0)<0){
      printf("Receive failed \n");
    }
    else printf ("odebrano \n");
    //if(player_messege[0]==NULL)continue;
    int code = (int)player_messege[0];
        if(code==1){
          printf("wchodze \n");
          draw_a_card(gi,ti->player_id);
          printf("wyszedlem\n");
        }
        if(code==2){
          raise_a_totem(gi,ti->player_id);
        }
    memset(&player_messege, 0, sizeof (player_messege));
    }
    delete_game(&Game_list,gi->game_id);
    printf("Player exited the game\n");
    pthread_exit(NULL);
}

int main(){
  //game variables
  int number_of_games=0;
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

    while(1) //create a singular game here
    {
        struct game_info *gra = malloc(sizeof(struct game_info));
        gra->game_id=++number_of_games;
        gra->numbers_of_players=MAX_PLAYERS;
        gra->order=0;
        int player_id = 0;
        add_game(&Game_list,gra);
        printf("%d\n",Game_list.bot->game_id);
        printf("%d\n",Game_list.top->game_id);
        //Accept call creates a new socket for the incoming connection
        addr_size = sizeof serverStorage;
        while(player_id<MAX_PLAYERS)
          {
            if(listen(serverSocket,50)==0)
                printf("Listening\n");
            else
                printf("Error\n");    
            newSocket = accept(serverSocket, (struct sockaddr *) &serverStorage, &addr_size);
                printf("weszlo\n");
            //increasing the numbers of players
            pthread_t listen_id;
            //pthread_t talk_id;
            gra->player_socket[player_id]=newSocket;
            struct thread_info *ti= malloc(sizeof(struct thread_info));
            ti->socket=newSocket;
            ti->gi=gra;
            ti->player_id=player_id++;   
            printf("Threading\n");
            if( (pthread_create(&listen_id, NULL, ListeningThread, ti) )!= 0 )
              printf("Failed to create thread\n");
          //if( (pthread_create(&talk_id, NULL, TalkingThread, &ti)) != 0 )
          //  printf("Failed to create thread\n");   

            pthread_detach(listen_id);
          //pthread_detach(talk_id);
          //pthread_join(thread_id,NULL);
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
  if(stos->top==NULL){
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
  printf("flaga1\n");
  struct game_info *tmp= lista->bot;
  printf("flaga2\n");
  while(tmp->game_id!=id)
    {
      if(tmp->next==NULL)return;
      tmp=tmp->next;
    }
  printf("flaga3\n");  
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
  printf("flaga4\n");
}

struct game_info find_a_game (struct List lista, int id){
  struct game_info empty;
  struct game_info tmp= *lista.bot;
  while(tmp.game_id!=id)
    {
      if(tmp.next==NULL)return empty;
      tmp=*tmp.next;
    }
  return tmp;  
}

void give_cards (struct game_info *gi,int from , int to){
  for (int i=0 ; i<gi->players[from].num_cards_shown ; i++){
    struct Card tmp =pull_s( &gi->players[from].cards_shown);
    push_q(&gi->players[to].cards_hidden,&tmp);
  }
  for (int i=0 ; i<gi->players[to].num_cards_shown ; i++){
    struct Card tmp =pull_s( &gi->players[to].cards_shown);
    push_q(&gi->players[to].cards_hidden,&tmp);
  }
  gi->players[to].num_cards_hidden+=gi->players[from].num_cards_shown;
  gi->players[to].num_cards_hidden+=gi->players[to].num_cards_shown;
  gi->players[from].num_cards_shown = gi->players[to].num_cards_shown = 0;
  gi->order=to;  

  send_game_state(gi);
}

void take_all (struct game_info *gi, int who){
  int cards_added = 0;
  for (int i=0 ; i<gi->numbers_of_players ; i++){
    for ( int j=0; j<gi->players[i].num_cards_shown ; j++){
      struct Card tmp =pull_s( &gi->players[i].cards_shown);
      push_q(&gi->players[who].cards_hidden,&tmp);
    }
    cards_added+=gi->players[i].num_cards_shown;
    gi->players[i].num_cards_shown=0;   
  }
  gi->players[who].num_cards_hidden+=cards_added;
  gi->order=who;

  send_game_state(gi);
}

void draw_a_card (struct game_info *gi, int who){
  if(gi->order!=who){
    printf("Gracz %d probowal sie ruszyc w kolejce gracza %d \n",who,gi->order);
    return;
  } 

  gi->players[who].num_cards_hidden--;
  gi->players[who].num_cards_shown++;
  struct Card *tmp = pull_q(&gi->players[who].cards_hidden);
  push_s(&gi->players[who].cards_shown,tmp);
  gi->order++;
  gi->order=gi->order%gi->numbers_of_players;
  printf("dobrano karte \n");
  send_game_state(gi);

}

void raise_a_totem (struct game_info *gi,  int player){
  printf("Gracz %d podniosl totem \n",player);
  int tmp=gi->players[player].cards_shown.top->card_id;
  for(int i=0; i<gi->numbers_of_players ; i++){
    if(i==player)continue;
    if(gi->players[i].cards_shown.top->card_id/4==tmp/4){
      give_cards(gi,player,i);
      return;  
    }
  }
  take_all(gi,player);
}

void send_game_state(struct game_info *gi){
  for(int i=0; i<gi->numbers_of_players ; i++){
    //printf("tworze wiadomosc dla gracza %d\n",i);
    char message[]= "000000000000000";
    message[0]=(char)gi->numbers_of_players;
    message[1]=(char)i;
    message[2]=(char)gi->order;
    //printf("flaga \n");
    for (int iter=0; iter<gi->numbers_of_players;iter++){
      message[3*iter+3]=(char)gi->players[iter].num_cards_hidden;
      message[3*iter+4]=(char)gi->players[iter].num_cards_shown;
      if(gi->players[iter].cards_shown.top==NULL) message[3*iter+5] = (char)75;
      else message[3*iter+5]=(char)gi->players[iter].cards_shown.top->card_id;
      //printf("flaga %d\n",iter);
    }
    printf("wysylam do %d \n",gi->player_socket[i]);
    send(gi->player_socket[i],message,sizeof(message),0);
  }
}

void start_game(struct game_info *gi){
 //zerowanie gry
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
      int id=rand()%number_of_cards;
      int j=0;
      for(int k=0; j<id; k++){
          if(was_chosen[k]==0)j++;
      }
      tmp->card_id=j;
      was_chosen[id]=1;  
      push_q(&gi->players[i].cards_hidden,tmp);
      gi->players[i].num_cards_hidden++;
      number_of_cards--;
    }
  printf("Cards drawn\n");
  send_game_state(gi);   
  }