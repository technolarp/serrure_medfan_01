# serrure_medfan_01
Cet objet simule une serrure d'aspect magique qui s'ouvre et se ferme à l'aide d'un code qui doit être réalisé en activant des capteurs reed avec un aimant. Les capteurs sont cachés sous des glyphes illuminés par des leds RGB. Le code est enregistré en mémoire. De nombreux paramètres sont configurables via un back office accessible en wifi. La logique du code est assez simple est peut-être facilement trouvée par essais successifs. par exemple, avec 4 capteurs reed, il y a 4*3*2*1 combinaison possibles, soit seulement 24 combinaisons à essayer avant de trouver la bonne.

Dans la configuration “usine”, la serrure fonctionne de la façon suivante. Il y a 4 glyphes composés de 4 capteurs magnétiques et 4 groupes de 2 leds. Pour ouvrir ou fermer la serrure, il faut trouver la bonne séquence d’activation pour chaque glyphe. Chaque glyphe doit être activé une seule fois et passant un aimant près du capteur magnétique.

Si le glyphe est correct, il s’illume en jaune et il faut tenter de trouver le suivant. Si le glyphe n’est pas le bon, tous les glyphes déjà activés sont désactivés et il faut refaire la séquence depuis le début.



**[Exemples](#Exemples)**  
**[Composants](#Composants)**  
**[Branchements](#Branchements)**  
**[BackOffice](#BackOffice)**  


## Exemples
Voici deux exemples de casing plus avancé :
 - Le premier est tout simple, il utilise l’anneau de leds du kit technoLARP.
 - Le deuxième est plus abouti, il utilise 9 leds et 9 capteurs reed avec un boitier en bois mdf peint.

Dans les 2 cas, les leds se trouvent derrière un diffuseur de lumière (une matière blanche translucide) et d’un masque (matière opaque) qui permet de former un texte visible à l’oeil nu!


<img width="300"  alt="serrure_medfan_exemple_1" src="https://github.com/user-attachments/assets/cadc7f39-f503-4a64-b4d4-6829afc173f2" />
<img width="300" alt="serrure_medfan_exemple_2" src="https://github.com/user-attachments/assets/16e42c1b-76b2-4fd0-8c64-456232a7e02c" />


## Composants
Vous aurez besoin pour monter la serrure medfan 01 :

|  | |
| :---------------- | :------: |
| Un kit technoLarp | <img src="./images/technolarp_pcb_wemos.jpg" width="200"> |
| Un câble micro-USB | <img src="./images/technolarp_cable_micro-usb.png" width="200"> |
| Un ruban ou d’un anneau de led ws2812b ou neopixel | <img src="./images/technolarp_leds_ws2812b_01.jpg" height="200"> |
| 4 capteurs magnétiques reed | <img src="./images/capteur_magnetique_reed.png" width="200"> |
| Un aimant | <img src="./images/aimant.png" width="200"> |
| Une batterie 18650 et son support | <img src="./images/technolarp_18650.jpg" width="200"> |

## Branchements
Connecter les composants sur le kit technoLARP comme sur cette photo :
<img width="740" height="884" alt="unnamed (2)" src="https://github.com/user-attachments/assets/86afda35-bcf4-4280-bbcd-5b59fee11da7" />

Ils vont sur les broches du MCP23017 (broches bleues/noires et broches vertes/noires) dans cet ordre :
*La numérotation des capteurs commence à 0 et non pas à 1*
|                 |    |
|-----------------|----|
| capteur reed 0  | a0 |
| capteur reed 1  | a1 |
| capteur reed 2  | a2 |
| capteur reed 3  | a3 |
| capteur reed 4  | a4 |
| capteur reed 5  | a5 |
| capteur reed 6  | a6 |
| capteur reed 7  | a7 |
| capteur reed 8  | b0 |
| capteur reed 9  | b1 |
| capteur reed 10 | b2 |
| capteur reed 11 | b3 |
| capteur reed 12 | b4 |
| capteur reed 13 | b5 |
| capteur reed 14 | b6 |
| capteur reed 15 | b7 |
**!! Attention au sens de numérotation des broches !!**
<img width="800" alt="unnamed (3)" src="https://github.com/user-attachments/assets/74ecd236-886d-4812-81c1-d85b1e8c6625" />

|                   |                   |
|-------------------|-------------------|
| **a7    <====    a0** | **b0   ====>     b7** |

Dans le kit technoLARP, il y a 4 capteurs reeds. Ils seront donc connectés sur les broches a0 à a3

Les capteurs ont 2 broches dupont femelles et se connectent de cette façon !
 - 1 fiche sur la broche verte ou bleue
 - la 2ème sur la broche noire au dessus (la couleur des fils n’a pas d’importance)

Exemple pour le capteur n°1
<img width="800"  alt="unnamed (4)" src="https://github.com/user-attachments/assets/369087a3-96b0-4f7d-a6ed-19d3136401d1" />

Exemple avec les 4 capteurs
<img width="752" alt="unnamed (5)" src="https://github.com/user-attachments/assets/253f0eca-7694-4ac9-8b8d-e81af6e85414" />


## Installation
Pour installer le firmware de l'objet, il faut suivre ce [tutorial](https://github.com/technolarp/technolarp.github.io/wiki/Installation-du-firmware)  


## BackOffice
Pour se connecter au back office de l'objet, il faut suivre ce [tutorial](https://github.com/technolarp/technolarp.github.io/wiki/Connexion-au-back-office-de-l'objet-via-le-wifi)  

<img width="608" height="922" alt="serrure_medfan_back-office" src="https://github.com/user-attachments/assets/53296d62-3844-4aaf-b9e3-07d6e23bd710" />

## Paramètres de gameplay

| Nom         | Descriptif                                                                                                                         |
|---------------------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| **Object name**         | Le nom de l’objet, composé de 1 à 20 lettres et chiffres                                                                                                                         |
| **Object ID**           | Un numéro d’identification de l’objet                                                                                                                                            |
| **Group ID**            | Un numéro d’identification du groupe de l’objet                                                                                                                                  |
| **nbSegments**          | Le nombre de segments du panneau. Un segment est un groupe de 1 à 5 leds. Nombre entre 1 et 20.                                                                                  |
| **ledParSegment**       | Le nombre de leds groupées pour chaque segment du panneau. Entre 1 et 5                                                                                                          |
| **ActiveLeds**          | Le nombre total de leds utilisées par le panneau. Ce chiffre est calculé automatiquement (= nbColonnes X nbSegments). Il ne doit pas dépasser 25                                 |
| **Brightness**          | La luminosité des leds, entre 0 (éteinte) et 255 (pleine intensité)                                                                                                              |
| **Scintillement**       | Le scintillement permet d’activer le scintillement des leds pour un effet visuel plus ou moins rapide. le slider permet de régler la vitesse du scintillement                    |
| **couleurs fermée**     | Choix de la couleur quand la serrure est fermée                                                                                                                                  |
| **couleurs ouverte**    | Choix de la couleur quand la serrure est ouverte                                                                                                                                 |
| **couleurs transition** | Choix de la couleur qui indique si un glyphe a été activé                                                                                                                        |
| **Taille code**         | Le nombre de glyphe à activer pour ouvrir la serrure. entre 1 et nbSegments. Les boutons permettent de réinitialiser le code à 1-2-3-4… ou de mettre un code aléatoire           |
| **Code**                | la séquence qui permet de fermer ou ouvrir la serrure. Dans cet exemple, il faudra activer le capteur reed n°2, puis n°0, puis n°1 et enfin n°3 pour ouvrir ou fermer la serrure |
| **statut Panneau**      | le statut actuel de l’objet. Il peut être  OUVERTE FERMEE OUVERTURE Les 3 boutons permettent de forcer l’état de la serrure                                                      |
| **Timeout Reset**       | le délai après lequel les glyphes activés sont éteints automatiquement                                                                                                           |
| **Debounce Time**       | un paramètre à n pas modifier                                                                                                                                                    |


