# robocoop

Arduino project to automatically close the door of the chicken coop at given time after sunset.


## Commandes USB
Parametres de connection:
- baudrate: 	115200
- line ending: 	CR+NL

### version
Renvoie la version du programme. (v 0.1 deployee le 15 mai).

### sunset
Sans parametre, renvoie l'heure du coucher de soleil du jour.
Parametres optionels:
* 1 parametre: numero du jour de l'annee (1-366).
* 2 parametres: Mois et Jour.


### offset
Sans parametre, renvoie le delai (offset) apres le coucher de soleil pour fermer la porte.

Avec un parametre, delais en minutes (0-15000), change la valeur du delai.

### date
Sans parametre, renvois la date et heure courante de la puce DS1307, au fuseau GMT (pas d'heure d'ete/hiver).

Avec les parametres YYYY MM DD HH mm SS, change la date et heure.
Example:
```
date 2018 05 15  14 55 00
```

### opentime
Sans parametres, renvoie l'heure d'ouverture de la porte.

Avec les parametre HH MM, change l'heure d'ouverture de la porte.
Example, pour que la porte s'ouvre a 8h00 GMT (10h Thiefosse):
```
opentime 8 00
```

### open
Ouvre la porte.

### close 
Ferme la porte.

### Help
Renvoie la liste des commandes.

