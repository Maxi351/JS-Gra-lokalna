from distutils.log import error
import errno
from pickle import TRUE
import pygame
import sys
import threading
import multiprocessing
from copy import deepcopy
from Button import Button
import time
import math
import socket


class Game_state:
    number_of_players=0
    player_id = 1
    which_player = 0
    top_card = [0,0,0,0]
    num_cards_hidden = [0,0,0,0]
    num_cards_shown = [0,0,0,0]

FLAGA_PODNIESIENIE_TOTEMU = 0
FLAGA_WYLOZENIA_KARTY = 1

#pygame.init()
pygame.font.init()

flags = {'gra_trwa': False,'connecting':False,'conection_online':False,'rozgrywka':False,'start_gra':False}
buttons = []
global_clock = pygame.time.Clock()

#global komunikacja
global_stan_gry = Game_state()
global_socket = None
server_addr = ('localhost', 2300)

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
            
            con = threading.Thread(target=listen_thread, args=(global_socket,))
            con.start()
            flags['conection_online'] = True
            
            return con
        except AttributeError as ae:
            print("Error creating the socket: {}".format(ae))
        except socket.error as se:
            print("Exception on socket: {}".format(se))

def listen_thread(sock):

    global global_stan_gry

    sock.settimeout(1)

    while flags['gra_trwa']:
        try:
            msg = sock.recv(1*15,socket.MSG_WAITALL)
        except TimeoutError:
            continue

        if msg == b'\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00' or len(msg)<15:
            continue

        print("Stan gry: ", msg)

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

        global_stan_gry = temp

    sock.close()


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

    while flags['gra_trwa'] and flags['rozgrywka']:
        events()
        stan_gry = global_stan_gry

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

        pygame.display.flip()

    flags['rozgrywka']=False


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
print("Zamykanie pygame")
pygame.quit()
print("Koniec gry")
sys.exit()