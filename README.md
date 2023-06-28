# Langton-ants

Solutie distribuita folosind MPI pentru rezolvarea problemei Langton's ants.


Jocul presupune existenta unei harti sub forma unei matrici de dimensiune H x W fiecare element avand
una din urmatoarele doua culori:
• Alb
• Negru
Simularea va fi executata pentru un numar dat de etape (pasi). Astfelpe unul dintre elementele matricei
se indentifica o furnica ce se poate deplasa in una dintre cele patru directii cardinalela fiecare pas al simularii,
dupa urmatoarele reguli:
• Daca se afla pe un element Albfurnica isi modifica direct,ia cu 90° dreapta (sens orar)schimba
culoarea patratului si avanseaza pe urmatorul patrat;
• Daca se afla pe un element Negrufurnica isi modifica direct,ia cu 90° stanga (sens trigonometric),
schimba culoarea patratului si avanseaza pe urmatorul patrat.
