import pygame

def default_fun():
    pass

class Button:
    def __init__(self,guzik_wysokosc,guzik_szerokosc,tekst,
        pos_x,pos_y,guzik_kolor1,guzik_kolor2,guzik_kolor3,tekst_kolor,
        ramka_szerokosc,fun = default_fun):
        self.guzik_wysokosc = guzik_wysokosc
        self.guzik_szerokosc = guzik_szerokosc
        self.tekst = tekst

        self.pos_x = pos_x
        self.pos_y = pos_y

        self.guzik_kolor1 = guzik_kolor1
        self.guzik_kolor2 = guzik_kolor2 
        self.guzik_kolor3 = guzik_kolor3 

        self.tekst_kolor = tekst_kolor
        self.ramka_szerokosc = ramka_szerokosc

        self.myfont_buttons = pygame.font.SysFont('Comic Sans MS', int(guzik_wysokosc * (1/2)))

        self.cover = False
        self.click = False
        self.fun = fun
        
    
    def draw(self,screen):

        guzik_rect = pygame.Rect(self.pos_x, self.pos_y, 
        self.guzik_szerokosc, self.guzik_wysokosc)

        pygame.draw.rect(screen,self.guzik_kolor1,guzik_rect)

        if self.click:
            srodek_kolor = self.guzik_kolor3
        else:
            srodek_kolor = self.guzik_kolor2


        pygame.draw.rect(screen,srodek_kolor,
            pygame.Rect(self.pos_x+self.ramka_szerokosc, self.pos_y+self.ramka_szerokosc,
            self.guzik_szerokosc-2*self.ramka_szerokosc, self.guzik_wysokosc-2*self.ramka_szerokosc))

        napis = self.myfont_buttons.render(self.tekst,True,self.tekst_kolor)
        screen.blit(napis,napis.get_rect(center=guzik_rect.center))
    
    def update_mouse_click(self,pos):
        x = pos[0]
        y = pos[1]

        if self.pos_x<x<self.pos_x+self.guzik_szerokosc and self.pos_y<y<self.pos_y+self.guzik_wysokosc:
            print(f"Naciśnięto {self.tekst}")
            self.click = True
    
    def update_mouse_unclick(self):
        if self.click:
            self.click = False
            self.fun()