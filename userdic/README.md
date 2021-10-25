# Tworzenie słowników użytkownika

Słownik użytkownika może zawierać:
* jednostki (dla liczb mianowanych)
* słowa lub dłuższe frazy

Załączony program ```makeudict.py``` (wymagany Python 3) tworzy źródłowy
plik dla języka C/C++. Dane zawarte w takim pliku mogą być zaimportowane
do microleny. Słownik użytkownika jest analizowany przed wbudowanym,
co pozwala na nadpisanie istniejących translacji.

## Struktura pliku wejściowego.

Plik wejściowy zawiera linie definiujące:
* deklaracje jednostek (rozpoczynające się od #unit lun #unitf)
* zasady wymowy słów i fraz


Ciąg znaków '//' oznacza komentarz (treść za tymi znakami zostanie zignorowana).
Puste linie są pomijane

### Zasady dotyczące tłumaczeń:

Tłumaczenia mogą zawierać słowa składające się z:

* małych liter alfabetu polskiego, z których po każdej może wystąpić
ciąg znaków ```~'``` oznaczający wymowę zgodną wyłącznie z konkretną
literą. Przykładowo: jeśli w słowie wystąpi ciąg "rz"" będzie on
potraktowany w następnej fazie przetwarzania albo jako fonetyczne 'ż',
albo jako dwa oddzielne fonrmy 'r' i 'z' zależnie od reguł tłumaczenia.
Wystąpienie ciągu "r~'z" oznacza, że litery r i z mają być traktowane
oddzielnie (np. mir~'za, mur~'zasichle)
* znaku @ oznaczającego fonem "schwa" (jak 'bit@ls' w tłumaczeniu Beatles)
* znaku podkreślenia, oznaczającego konieczność rozdzielenia fonemów
(szczególnie ważne przy oddzielnej wymowie następujących po sobie samogłosek)
* w tłumaczeniach fraz (ale nie jednostek) może wystąpić również znak %, oznaczający
umieszczenie w tym miejscu odpowiedniego stringu dopasowanego do alternatywy

Słowa mogą być poprzedzone markerem sterowania wymową. Marker umieszczony jest
w nawiasach kwadratowych i może zawierać znaki:
* ```u``` oznaczający wyraz nieakcentowany
* cyfra z zakresu od 1 do 4 oznaczająca, na którą sylabę od końca pada akcent.
Jeśli liczba będzie większa niż ilość sylab w słowie, zostanie obcięta do ilości sylab.
Jeśli na pozycji wypada shwa, brana jest pod uwagę poprzednia sylaba (schwa nigdy
nie jest akcentowana).

Przykładowo - zapis tłumaczenia:

```
[u]na [1]co
```
oznacza, że będzie on się składał z nieakcentowanego 'na' oraz akcentowanego 'co'.

## Deklaracje jednostek

Deklaracja składa się z:
* słowa #unit lub #unitf (oznaczające odpowiednio męską i żeńską odmianę liczebników)
* słowa odpowiadające jednostce (np. kΩ, msec itp)
* czterech form oddzielonych znakami '|', oznaczającymi odpowiednio:
  * liczba pojedyncza (jeden kiloom)
  * liczba podwójna (dwa kiloomy)
  * liczba mnoga (pięć kiloomów)
  * liczba ułamkowa (0.5 kilooma)

Tłumaczenia muszą być zgodne z zasadami pisania tłumaczeń

I tak np. tworząc interfejs dla statku gwiezdnego jednostką może być ```pps``` (parsek na sekundę),
linia opisująca jednostkę będzie wyglądać następująco:

```
#unit pps parsek na sekundę|parseki na sekundę|parseków na sekundę|parseka na sekundę
```

Dla superszybkiego lądownika mogą być to np. mile na sekundę (mps), czyli:

```
#unitf mps mila na sekundę|mile na sekundę|mil na sekundę|mili na sekundę
```

Istniejący w słowniku zapis kilooma wygląda następująco (rozdzielenie podwójnego 'o'):

```
#unit kΩ kilo_om|kilo_omy|kilo_omów|kilo_oma
```

### Zasady wymowy słów i fraz

Każda z zasad składa się z:

* wzorca
* tłumaczenia
* markera stosowania zasad

Wzorzec może zawierać wielkie i małe litery, cyfry, znaki dopasowujące,
klasy znaków, alternatywy oraz znaki przestankowe nie kolidujące ze znakami specjalnymi.
Jeśli we wzorcu wystąpi wielka litera, zostanie ona dopasowana do wielkiej
litery w treści. W przypadku małej litery zostanie ona dopazowana zarówno do wielkiej,
jak i małej litery.

Znaki dopasowujące to:

* "+" (plus) oznaczający co najmniej jedną spację
* "_" (znak podkreślenia) oznaczający zero lub więcej spacji
* "'" (apostrof) oznaczający apostrof wraz z ewentualnymi następującymi spacjami
* "`" (backtick) oznaczający opcjonalny apostrof wraz z otaczającymi spacjami
* "~" (tylda) oznaczający myślnik lub ciąg spacji
* "~~" (podwójna tylda) oznaczająca dowolny (róœnież pusty) ciąg myślników i spacji

Przykładowo:

* ```Joe'go``` będzie dopasowane do "Joe'go", "JOE' go" ale nie "joe'go"

* ```Joe`go``` będzie również dopasowane do "Joe 'go" i "Joego"

Klasy znaków umieszczane są w nawiasach kwadratowych i oznaczają dowolny
ze znaków z klasy. Przykładowo:

```gal[áa]pagos``` będzie dopasowane do "galapagos" i "galápagos"

Alternatywy to ciągi znaków umieszczone w nawiasach i rozdzielone znakami "|".
Przykładowo:

```joe`(go|mu)``` będzie pasować do "Joe'go", "Joemu" i tak dalej.

Dodatkowo na końcu wzorca może być zastosowana gwiazdka "*", oznaczająca
dowolny ciąg liter do końca wyrazu.

W przypadku tłumaczenia, zastosowanie znaku "%" powoduje wstawienie w
dane miejsce w tłumaczeniu ciągu znaków odpowiadających alternatywie
lub gwiadce i zamienonego na małe litery. Kolejne znaki "%" odpowiadają
kolejnym alternatywom.

Przykładowo:

```Jim(a|owi|em|) dżi% ```

oznacza, że:
* "jima" zamienione będzie na "dżima"
* "jim" zamienione będzie na "dżim"
* "jimem" zamienione będzie na "dżimem"
* "jimowi" zamienione będzie na "dżimowi"


## Markery

Markery występują opcjonalnie na końcu linii i są oddzielone znakiem dolara.
Mogą zawierać:

* "S" oznaczający, że dane słowo jest skrótem. W takim przypadku
tłumaczenie musi być puste i nie może wystąpić żaden inny marker
* Cyfra od 1 do 4 oznaczająca pozycję akcentu od końca. W takim przypadku
tłumaczenie może być puste, ale wtedy rozwinięciem wzorca mogą być wyłącznie litery
* "e" oznaczające, że dany wzorzec będzie dopasowany wyłącznie na końcu frazy.

Przykładowo:

* ```galapagos $3``` - oznacza akcent na trzeciej sylabie od końca
* ```col[oó]n kolon $1``` - podaje tłumaczenie wraz z wymuszeniem akcentu na ostatniej sylabie
* ```oir $S``` - informuje, że słowo "oir" jest skrótem i należy je przeliterować
* ```ale+nie [u]ale [1]nie $e``` - informuje, że występujące na końcu frazy "ale nie" należy wymówić akcentując słowo "nie"

### UWAGA

Zasady są dopasowywane kolejno na zasadzie "pierwszy pasujący wzorzec kończy dopasowanie"

## Wywołanie programu:

```python3 makeudict.py <plik_wejściowy> [-out <plik_wynikowy>] [-name <nazwa struktury>]```

W pliku wynikowym zostaną zawarte definicje tablic ```userdic_units``` i ```userdic_lines```.
Jeśli podamy parametr -name, napis ```userdic``` zostanie zamieniony na podany.
Jeśli nie podamy parametru -out, plik wynikowy nie będzie utworzony, wynik
będzie jedynie wypisany na konsoli.

Przykładowo:

```python makeudict.py scifi.txt -name sf -out sfdic.h```

Utworzy plik wynikowy sfdic.h z zawartymi definicjami ```sf_units``` i ```sf_lines```.

## Zastosowanie w aplikacji

Aby dołączyć powstałe w ten sposób tablice do aplikacji, należy użyć funkcji:

```void microlena_setUserDict(const char * const *units, const char * const *dict);```

podając odpowiednie parametry. Każdy z parametrów może być NULL. Funkcję można wywołać
w dowolnym momencie, umieszcza ona po prostu wskaźniki do tablic w zmiennych statycznych.

## Plik demo

Można wypróbować działanie na załączonym pliku scifi.txt:

```
#unit pps parsek na sekundę|parseki na sekundę|parseków na sekundę|parseka na sekundę

warp łorp // tak lata Enterprise
Alderaan alderan $3 // ojczysta planeta pewnej księżniczki
Alderaan(em|ie|u|owi) alderan%
Galapagos $3
oir $S // Organizacja Istot Rozumnych
oir-(em|u|owi|ze) o~'i~'er%
```
