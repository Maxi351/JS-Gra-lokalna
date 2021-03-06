from pickle import TRUE
import pygame
import sys
import threading
from copy import deepcopy
from Button import Button
import time
import math
import socket
from copy import deepcopy


class Game_state:
    number_of_players=0
    player_id = 1
    which_player = 0
    top_card = [75,75,75,75]
    num_cards_hidden = [-1,-1,-1,-1]
    num_cards_shown = [0,0,0,0]

FLAGA_PODNIESIENIE_TOTEMU = 2
FLAGA_WYLOZENIA_KARTY = 1

#pygame.init()
pygame.font.init()

flags = {'gra_trwa': False,'connecting':False,'conection_online':False,'rozgrywka':False,'start_gra':False}
buttons = []
global_clock = pygame.time.Clock()

lock = threading.Lock()


#global komunikacja
global_stan_gry = Game_state()
global_socket = None
server_addr = ('127.0.0.1', 1100)

SCREEN_HEIGHT = 700
SCREEN_WIDTH = 700


screen = pygame.display.set_mode([SCREEN_WIDTH, SCREEN_HEIGHT])

def connect_to_server():
    global global_socket
    global server_addr

    global_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

    while flags['gra_trwa']:
        try:
            global_socket.connect(server_addr)
            print("Connected to {:s}".format(repr(server_addr)))

            flags['conection_online'] = True

            con = threading.Thread(target=listen_thread, args=(global_socket,))
            con.start()

            
            return con
        except AttributeError as ae:
            print("Error creating the socket: {}".format(ae))
        except socket.error as se:
            print("Exception on socket: {}".format(se))
        events()
    return None

def log_out():
    try:
        global_socket.send(chr(13).encode())
    except BaseException as e:
        print(f"error log_out {e}")

def listen_thread(sock):

    global global_stan_gry

    sock.settimeout(1)

    last = []

    while flags['gra_trwa'] and flags['conection_online']:
        check_connection(sock)

        # while(last!=[110,110,110]):
        #     try:
        #         msg = sock.recv(1,socket.MSG_WAITALL) 
        #     except TimeoutError:
        #         continue
        #     last.append(ord(msg.decode()[0]))
        #     if len(last) >3:
        #         last.pop(0)
        #     print(last)
        #     check_connection(sock)

        try:
            msg = sock.recv(1,socket.MSG_WAITALL) 
        except TimeoutError:
            continue
        
        if len(msg)>0:
            last.append(ord(msg.decode()[0]))
        
        if len(last) >3:
            last.pop(0)

        #print(last)

        if last!=[110,110,110]:
            continue

        last = []

        if flags['conection_online'] == False:
            continue
        try:
            msg = sock.recv(1*15,socket.MSG_WAITALL) 
        except TimeoutError:
            continue
        # except:
        #     flags['conection_online'] = False
        #     break

        if msg == b'\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00' or len(msg)<15:
            continue

        print("Stan gry: ", msg)

        listaM = []
        for i in msg:
            listaM.append(int(i))
        print(listaM)

        msg = msg.decode()

        temp = Game_state()
        temp.number_of_players = ord(msg[0])
        temp.player_id = ord(msg[1])
        temp.which_player = ord(msg[2])

        list_top = []
        list_hidden = []
        list_shown = []

        for i in range(temp.number_of_players):
            list_hidden.append(ord(msg[3+i*3+0]))
            list_shown.append(ord(msg[3+i*3+1]))
            list_top.append(ord(msg[3+i*3+2]))

        temp.top_card = list_top
        temp.num_cards_hidden = list_hidden
        temp.num_cards_shown = list_shown
        with lock:
            global_stan_gry = temp
        # print(global_stan_gry.num_cards_hidden)
        # print(global_stan_gry.num_cards_shown)
        # print(global_stan_gry.top_card)
    if flags['conection_online']:
        log_out()
    print("Zmkniento listen_thread")
    print(flags['gra_trwa'], flags['conection_online'])
    sock.close()

def check_connection(sock):
    
    try:
        sock.send(chr(13).encode())
    except BrokenPipeError:
        flags['conection_online'] = False
    except BaseException as e:
        print(f'Error podczas chceck connection {e}')




def load_cards(size_x,size_y = None):

    if size_y is None:
        size_y = size_x

    path = "cards/Card_{}.png"

    cards = []

    for i in range(76):
        c = pygame.image.load(path.format(i))
        c.convert()
        c = pygame.transform.scale(c, (size_x, size_y))

        cards.append(c)
    
    return cards

def pop_up(screen,x_size,y_size,tekst,kolor=(128,13,64),kolor_ramka = (0,0,0),
    tekst_kolor=(255,255,255),czcionka_stosunek = 5):

    w, h = pygame.display.get_surface().get_size()


    rectan  = pygame.Rect(0, 0, x_size,y_size)
    rectan.center = (w/2,h/2)
    pygame.draw.rect(screen,(0,0,0),rectan)

    ramka = y_size/30
    rectan2  = pygame.Rect(0, 0, x_size-ramka*2,y_size-ramka*2)
    rectan2.center = (w/2,h/2)
    pygame.draw.rect(screen,kolor,rectan2)

    myfont = pygame.font.SysFont('Comic Sans MS', int(y_size/5))
    napis = myfont.render(tekst,True,tekst_kolor)
    screen.blit(napis,napis.get_rect(center=rectan2.center))

def button_exit_fun():
    flags['gra_trwa'] = False

def button_NewGame_fun():
    print("Startujemy gre")
    flags['start_gra']=True

def events():
    for event in pygame.event.get():
        #print(event)
        if event.type == pygame.QUIT:
            flags['gra_trwa'] = False
        if event.type == pygame.MOUSEBUTTONDOWN:
            if event.button == 1:
                for b in buttons:
                    b.update_mouse_click(event.pos)
        if event.type == pygame.MOUSEBUTTONUP:
            if event.button == 1:
                for b in buttons:
                    b.update_mouse_unclick()
        if event.type == pygame.KEYDOWN:
            if event.key == 27: #esc
                if flags['rozgrywka']:
                    # TODO "czy napewno?"
                    flags['gra_trwa']=False
            elif event.key == 32: #spacja
                if flags['rozgrywka']:
                    try:
                        ns = global_socket.send(chr(FLAGA_PODNIESIENIE_TOTEMU).encode())
                        print("wyslano podniesienie totemu")
                    except BaseException as err:
                        print("Error przy podnoszeniu totemu: {}".format(err))
            elif event.key == 110: #n
                if flags['rozgrywka']:
                    try:
                        ns = global_socket.send(chr(FLAGA_WYLOZENIA_KARTY).encode())
                        print("wyslano wylozenie karty") 
                    except BaseException as err:
                        print("Error wylozeniu karty totemu: {}".format(err))





def draw_logo(screen):
    screen.blit(background_pic, (0, 0))

def draw_connecting_screen(screen,n):
    draw_logo(screen)
    draw_sciemnienie(screen)

    tekst = "Connecting" + '.'*(n%4)
    color = (255,255,255)


    myfont = pygame.font.SysFont('Comic Sans MS', int(SCREEN_HEIGHT/7))
    napis = myfont.render(tekst,False,color)
    screen.blit(napis,(0,0))

def draw_sciemnienie(screen):
    screen.blit(transparent_screen, (0, 0))

def connecting_screen(screen):

    i = 0

    while flags['gra_trwa'] and flags['connecting']:
        draw_connecting_screen(screen,i)
        i+=1        
        pygame.display.flip()
        global_clock.tick(5)

def draw_arrow(screen, colour, start, end,width=2):
    pygame.draw.line(screen,colour,start,end,width)
    rotation = math.degrees(math.atan2(start[1]-end[1], end[0]-start[0]))+90

    size = width*1
    pygame.draw.polygon(screen, colour, (
        (end[0]+size*math.sin(math.radians(rotation)), end[1]+size*math.cos(math.radians(rotation))),
        (end[0]+size*math.sin(math.radians(rotation-120)), end[1]+size*math.cos(math.radians(rotation-120))), 
        (end[0]+size*math.sin(math.radians(rotation+120)), end[1]+size*math.cos(math.radians(rotation+120)))
        ))

def game(screen):

    global global_stan_gry
    #screen = pygame.display.set_mode([1600, 1000])
    screen = pygame.display.set_mode((0, 0), pygame.FULLSCREEN)

    w, h = pygame.display.get_surface().get_size()

    CARD_SIZE = h/4
    CARDS_RAMKA_OFFSET = h/20 +CARD_SIZE/2

    color_tekstu = (255,255,255)

    myfont = pygame.font.SysFont('Comic Sans MS', int(SCREEN_HEIGHT/7))

    cards = load_cards(CARD_SIZE)

    logo_pic = pygame.image.load('Grafiki/Logo.png')
    logo_pic = pygame.transform.scale(logo_pic, (h/4, h/4))

    arrow_pic = pygame.image.load('Grafiki/Arrow.png')
    arrow_pic = pygame.transform.scale(arrow_pic, (h/2,h/3))

    transparent_color = (0, 0, 0)
    logo_pic.set_colorkey(transparent_color)
    arrow_pic.set_colorkey(transparent_color)

    arrow_tab = []
    arrow_tab.append(pygame.transform.rotate(arrow_pic,90))
    arrow_tab.append(arrow_pic)
    arrow_tab.append(pygame.transform.rotate(arrow_pic,90*3))
    arrow_tab.append(pygame.transform.flip(arrow_pic,True,False))

    pozycja_kart = [(w/2,h-CARDS_RAMKA_OFFSET),(CARDS_RAMKA_OFFSET,h/2),(w/2,CARDS_RAMKA_OFFSET),(w-CARDS_RAMKA_OFFSET,h/2)]

    pozycja_liczb = [(w/2+CARD_SIZE/1.8,h-CARDS_RAMKA_OFFSET),
                        (0+CARDS_RAMKA_OFFSET/2,h/2+CARD_SIZE/1.8),
                        (w/2+CARD_SIZE/1.8,CARDS_RAMKA_OFFSET),
                        (w-CARD_SIZE,h/2+CARD_SIZE/1.8)]


    is_winner = -1

    while flags['gra_trwa'] and flags['rozgrywka'] and flags['conection_online'] and is_winner == -1:
        events()
        with lock:
            stan_gry = deepcopy(global_stan_gry)
        id_gracz = stan_gry.player_id

        stan_gry.which_player = stan_gry.which_player-stan_gry.player_id
        if(stan_gry.which_player<0):
            stan_gry.which_player+=4
        
        while(stan_gry.player_id>0):
            stan_gry.num_cards_hidden.append(stan_gry.num_cards_hidden[0])
            stan_gry.num_cards_hidden.pop(0)

            stan_gry.num_cards_shown.append(stan_gry.num_cards_shown[0])
            stan_gry.num_cards_shown.pop(0)

            stan_gry.top_card.append(stan_gry.top_card[0])
            stan_gry.top_card.pop(0)

            stan_gry.player_id-=1

        
        #tlo
        screen.fill((122,0,17))
        
        screen.blit(logo_pic,logo_pic.get_rect(center=(w/2,h/2)))

        #arrow
        screen.blit(arrow_tab[stan_gry.which_player],arrow_tab[stan_gry.which_player].get_rect(center=(w/2,h/2)))

        #print(stan_gry.which_player)



        #karty graczy
        for i in range(4):
            
            screen.blit(cards[stan_gry.top_card[i]],cards[stan_gry.top_card[i]].get_rect(center=pozycja_kart[i]))
        

        #numery przy kartach
        for i in range(4):
            tekst = f'{stan_gry.num_cards_hidden[i]} / {stan_gry.num_cards_shown[i]}'
            napis = myfont.render(tekst,False,color_tekstu)

            screen.blit(napis,pozycja_liczb[i])

        #id gracza
        napis = myfont.render(f"id: {id_gracz}",False,color_tekstu)
        screen.blit(napis,(0,0))
        pygame.display.flip()

        for i in range(4):
            if stan_gry.num_cards_hidden[i] == 0 and stan_gry.num_cards_shown[i]==0:
                is_winner=i
                break

    screen.fill((0,0,0))
    if flags['conection_online'] == False and flags['gra_trwa'] and flags['rozgrywka']:
        pic = pygame.image.load('Grafiki/Disconnect.jpg')
        pic = pygame.transform.scale(pic, (h*(16/9), h))
        screen.blit(pic,pic.get_rect(center=(w/2,h/2)))
        pygame.display.flip()

        f = True
        while f:
            for event in pygame.event.get():
                if event.type == pygame.MOUSEBUTTONDOWN or event.type == pygame.KEYDOWN:
                    print("Koniec")
                    f = False

 
    else:
        #log_out()
        if is_winner!= -1:

            if is_winner == stan_gry.player_id:
                pic = pygame.image.load('Grafiki/You_win.jpg')
                pic = pygame.transform.scale(pic, (h*(16/9), h))
            else:
                pic = pygame.image.load('Grafiki/You_lose.png')
                pic = pygame.transform.scale(pic, (h*(16/9), h))
            
            screen.blit(pic,pic.get_rect(center=(w/2,h/2)))
            pygame.display.flip()


            f = True
            while f:
                for event in pygame.event.get():
                    if event.type == pygame.MOUSEBUTTONDOWN or event.type == pygame.KEYDOWN:
                        print("Koniec")
                        f = False
        

    

    flags['rozgrywka']=False
    flags['gra_trwa'] = False


def start_game(screen):
    flags['start_gra']=False

    global buttons

    buttons_temp = buttons
    buttons = []

    flags['connecting'] = True
    x = threading.Thread(target=connecting_screen, args=(screen,))
    x.start()

    #connecting part
    c = connect_to_server()
    flags['connecting']=False
    
    
    # while flags['gra_trwa'] and flags['connecting']:
    #     events()
    #     time.sleep(1)
    #     flags['connecting']=False
    
    x.join()
    if c is not None:
        flags["rozgrywka"] = True
        game(screen)

        print("Zamykanie polaczenia")
        c.join()

    buttons = buttons_temp

def generate_menu():

    odstep_guziki = SCREEN_HEIGHT/70

    # button declare
    guzik_wysokosc = SCREEN_HEIGHT/10
    guzik_szerokosc = SCREEN_WIDTH/2.5
    
    tekst = "New game"

    pos_x = SCREEN_WIDTH/2-guzik_szerokosc/2
    pos_y = SCREEN_HEIGHT/3-guzik_wysokosc/2

    guzik_kolor1 = (0,0,0)
    guzik_kolor2 = (255,255,32)
    guzik_kolor3 = (255,255,164)

    tekst_kolor = (0,0,0)

    ramaka_szerokosc = 5

    button = Button(guzik_wysokosc,guzik_szerokosc,tekst,pos_x,pos_y,
        guzik_kolor1,guzik_kolor2,guzik_kolor3,tekst_kolor,ramaka_szerokosc)

    button.fun = button_NewGame_fun 
    
    buttons.append(button)
    

    pos_y+= guzik_wysokosc + odstep_guziki
    tekst = "Exit"

    button = Button(guzik_wysokosc,guzik_szerokosc,tekst,pos_x,pos_y,
        guzik_kolor1,guzik_kolor2,guzik_kolor3,tekst_kolor,ramaka_szerokosc)

    button.fun = button_exit_fun

    buttons.append(button)

def draw_menu(screen):
    draw_logo(screen)
    draw_sciemnienie(screen)

    for b in buttons:
        b.draw(screen)


background_pic = pygame.image.load('Grafiki/Background.png')
background_pic.convert()


background_pic = pygame.transform.scale(background_pic, (SCREEN_WIDTH, SCREEN_HEIGHT))

transparent_screen = pygame.Surface((SCREEN_WIDTH, SCREEN_HEIGHT), pygame.SRCALPHA, 32)
pygame.draw.rect(transparent_screen,(0,0,0,128),pygame.Rect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT))


generate_menu()


flags['gra_trwa']=True

while flags['gra_trwa']:

    events()
    draw_menu(screen)
    pygame.display.flip()
    if(flags['start_gra']):
        start_game(screen)
        screen = pygame.display.set_mode([SCREEN_WIDTH, SCREEN_HEIGHT])

print("Zamykanie pygame")
pygame.quit()
print("Koniec gry")
sys.exit()