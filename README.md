led_matrix
==========

Journal lumineux, matrices de LED 8x8, MAX7219 - Scrolling LED matrix display - Raspberry Pi

##Objectif
Ce journal a �t� d�marr� pour servir de d�monstration lors de la journ�e Raspberry Pi
au CERN � Gen�ve le 12 avril 2014.

.. image:: https://raw.github.com/framboise314/led_matrix/master/docs/img_01.jpg

    :alt: Le journal lumineux en action
    
    :align: center

##Pr�requis
Ce programme est destin� � l'utilisation sur un Raspberry Pi avec un syst�me d'exploitation Raspbian.
Les modules d'affichage utilis�es sont des mod�les �quip�s de circuits MAX7219 et de matrices � LED 8x8. 
Le programme g�re de 1 � N modules (le nombre de modules est pass� en param�tre sur la ligne de commande).

##Utilisation
Pour utiliser le programme, clonez le r�pertoire source sur votre machine
puis ex�cutez la commande `make` dans le r�pertoire `src`.

```bash
git clone https://github.com/framboise314/led_matrix.git
cd led_matrix/src
make
```

Il vous reste ensuite � ex�cuter le programme en lui passant le nombre de modules d'affichage � piloter (ici 6).

```
sudo ./led_matric 6 

```