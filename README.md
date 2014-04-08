led_matrix
==========

Journal lumineux, matrices de LED 8x8, MAX7219 - Scrolling LED matrix display - Raspberry Pi

##Objectif
Ce journal a été démarré pour servir de démonstration lors de la journée Raspberry Pi
au CERN à Genève le 12 avril 2014.

.. image:: https://raw.github.com/framboise314/led_matrix/master/docs/img_01.jpg

    :alt: Le journal lumineux en action
    
    :align: center

##Prérequis
Ce programme est destiné à l'utilisation sur un Raspberry Pi avec un système d'exploitation Raspbian.
Les modules d'affichage utilisées sont des modèles équipés de circuits MAX7219 et de matrices à LED 8x8. 
Le programme gère de 1 à N modules (le nombre de modules est passé en paramètre sur la ligne de commande).

##Utilisation
Pour utiliser le programme, clonez le répertoire source sur votre machine
puis exécutez la commande `make` dans le répertoire `src`.

```bash
git clone https://github.com/framboise314/led_matrix.git
cd led_matrix/src
make
```

Il vous reste ensuite à exécuter le programme en lui passant le nombre de modules d'affichage à piloter (ici 6).

```
sudo ./led_matric 6 

```