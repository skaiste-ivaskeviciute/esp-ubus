# ESP mikrovaldiklių valdymo modulis 
Darbo tikslas – sukurti programą, kuri leistų valdyti prie maršrutizatoriaus prijungtus ESP mikrovaldiklius naudojant UBUS: gauti prijungtų ESP mikrovaldiklių sąrašą, įjungti ar išjungti GPIO prievadus, bei nuskaityti prijungtus sensorius. 

Darbo užduotys:
1)	Mikrovaldiklio prievadams valdyti naudoti esp_control_over_serial programą: https://github.com/janenasl/esp_control_over_serial
2)	Paruošti UBUS metodus ESP mikrovaldikliams valdyti. Metodas „devices“ turėtų grąžinti informaciją apie prijungtus ESP (jų USB port‘ą, tiekėjo ID, bei produkto ID). Metodai „on“ ir „off“ turėtų įjungti/išjungti prie nurodyto port‘o prijungto mikrovaldiklio nurodytą prievadą. Metodas „get“ turėtų gauti prie mikrovaldiklio prijungto jutiklio informaciją, nurodžius mikrovaldiklio port‘ą, jutiklio prievadą, modelį bei jutiklio tipą.
3)	Visas žinutes grąžinti JSON formatu, naudojant „blob messages“ biblioteką. Įvykus klaidai, grąžinti UBUS klaidos žinutę.
4)	Paruošti OpenWRT paketą: programa turėtų automatiškai pasileisti, pasileidus maršrutizatoriui, bei turėtų veikti be UCI konfigūracijos failo.
