\NeedsTeXFormat{LaTeX2e}
\ProvidesClass{template/socthesis}[2026 Document class for czech SOC thesis]
\LoadClass[11pt, a4paper, oneside]{book}

% \RequirePackage[utf8]{inputenc}
\RequirePackage{fontspec}
\RequirePackage{inputenc}
\RequirePackage[czech]{babel}
\RequirePackage{luavlna}
\RequirePackage{graphicx}
\RequirePackage{tabularx}
\RequirePackage{array}
\RequirePackage[bookmarksopen,colorlinks,plainpages=false,linkcolor=black,urlcolor=blue,citecolor=black,filecolor=black,menucolor=black,unicode=true,breaklinks]{hyperref}
\RequirePackage{csquotes}
\RequirePackage[backend=biber,bibstyle=numeric,sorting=none,date=long,dateabbrev=false,texencoding=utf8,bibencoding=utf8,style=iso-numeric]{biblatex}
\RequirePackage{caption}
\RequirePackage{parskip}
\RequirePackage[margin=2.5cm, bindingoffset=1cm]{geometry}
\RequirePackage{titlesec}

\titleformat{\chapter}[display]
  {\normalfont\huge\bfseries}{\chaptertitlename\ \thechapter}{20pt}{\Huge}

% Nastavení mezer: {příkaz}{mezera ZLEVA}{mezera NAD}{mezera POD}
\titlespacing*{\chapter}{0pt}{0pt}{20pt}

\newcommand{\B}{\textbf} % tučné písmo
\newcommand{\It}{\textit} % kurzíva

%%%%%% ROZMĚRY

% posun textu kvůli kroužkové vazbě
%\addtolength{\textwidth}{mm}
% \addtolength{\hoffset}{4mm}

\setlength{\intextsep}{5mm} % nastavení mezery okolo obrázků
\setlength{\marginparwidth}{2cm}
\marginparsep=15pt
\linespread{1.3}
\unitlength=1mm % nastavení volby jednotek

%%%%%% ROVNICE
\DeclareCaptionType{listedequation}[Rovnice][Seznam rovnic]
%\captionsetup[listedequation]{labelformat = parens}

\DeclareCaptionType{code}[Kód][Seznam kódů]

%% hyperref \autoref nazvy
\def\sectionautorefname{Kapitola}
\def\figureautorefname{Obrázek}
\def\tableautorefname{Tabulka}

%%%%%% PROMĚNNÉ
\def\titlecz#1{\def\@titlecz{#1}}
\def\titleen#1{\def\@titleen{#1}}
\def\author#1{\def\@author{#1}}
\def\school#1{\def\@school{#1}}
\def\region#1{\def\@region{#1}}
\def\mentor#1{\def\@mentor{#1}}
\def\mentorstatement#1{\def\@mentorstatement{#1}}
\def\placefooter#1{\def\@placefooter{#1}}

\RequirePackage{xstring}
\newcommand{\field}[1]{%
\IfStrEqCase{#1}{
    {1}{\def\@field{1. Matematika a~statistika}}
    {2}{\def\@field{2. Fyzika}}
    {3}{\def\@field{3. Chemie}}
    {4}{\def\@field{4. Biologie, geografie a~geologie}}
    {5}{\def\@field{5. Molekulární biologie}}
    {6}{\def\@field{6. Zdravotnictví}}
    {7}{\def\@field{7. Zemědělství, potravinářství, lesní a~vodní hospodářství}}
    {8}{\def\@field{8. Ochrana a~tvorba životního prostředí}}
    {9}{\def\@field{9. Strojírenství, hutnictví a~doprava}}
    {10}{\def\@field{10. Elektrotechnika, elektronika a~telekomunikace}}
    {11}{\def\@field{11. Stavebnictví, architektura a~design interiérů}}
    {12}{\def\@field{12. Tvorba učebních pomůcek, didaktická technologie}}
    {13}{\def\@field{13. Ekonomika a~řízení}}
    {14}{\def\@field{14. Pedagogika, psychologie, sociologie a~problematika volného času}}
    {15}{\def\@field{15. Teorie kultury, umění a~umělecké tvorby}}
    {16}{\def\@field{16. Historie}}
    {17}{\def\@field{17. Filozofie, politologie a~ostatní humanitní a~společenskovědní obory}}
    {18}{\def\@field{18. Informatika}}
  }
  [Neznámé číslo oboru, upravte \\field{} příkaz!]
}

\titleen{}
\region{Pražský}
\placefooter{Praha \the\year}
\mentorstatement{\@mentor}

%%% Titulní strany
\renewcommand{\maketitle}{%
    % \pagestyle{empty}
    % \voffset = -20mm % posun začátku textu výš
    % \enlargethispage{40mm} % zvětší oblast tisku pro tuto stránku
    % \begin{center}
    %     {\large \B{STŘEDOŠKOLSKÁ ODBORNÁ ČINNOST}} \\
    %     \vspace{5mm}
    %     \normalsize \B{Obor: \@field} \\
    %
    %     \vspace{40mm}
    %
    %     {\LARGE \B{\@titlecz}} \\
    %
    %     \vfill
    %
    %         {\Large \B{\@author}} \\
    %
    %     \vspace{20mm}
    %
    %     {\large \B{\@placefooter}}
    % \end{center}
    % \newpage

    \pagestyle{empty}
    % \enlargethispage{40mm} % zvětší oblast tisku pro tuto stránku
    \begin{center}
        {\Large \B{STŘEDOŠKOLSKÁ ODBORNÁ ČINNOST}} \\
        {\large \B{Obor č. 18: Informatika}}

        \vspace{40mm}

        {\Large \B{\expandafter\MakeUppercase\expandafter{\@titlecz}}} \\
        \vspace{20mm}
        {\Large \B{\expandafter\MakeUppercase\expandafter{\@titleen}}} \\

        \vfill

        \begin{table}[h]
        \setlength{\tabcolsep}{12pt}
        \renewcommand{\arraystretch}{1.0}
        \linespread{1.0}
        \begin{tabularx}{\textwidth}{r >{\raggedright}p{90mm} c}
            \large \B{AUTOŘI} & \large \B{\@author} & \\
            \large \B{ŠKOLA} & \large \B{\@school} & \\
            \large \B{KRAJ} & \large \B{\@region} & \\
            \large \B{KONZULTANT} & \large \B{\@mentor} & \\
            \large \B{OBOR} & \large \B{\@field} & \\
            \large \B{ROK} & \large \B{2026} & \\
        \end{tabularx}
        \setlength{\tabcolsep}{6pt}
        \renewcommand{\arraystretch}{1.0}
        \end{table}

        \vspace{10mm}

        % {\large \B{\@placefooter}}
    \end{center}
    \newpage
    \voffset = 0mm
    \pagestyle{plain}
}

%%% Prohlášení
\newcommand{\makecopyrightstatement}[1]{
    % \pagestyle{empty}
    %
    % ~
    % \vfill

    \section*{Prohlášení}
    % Prohlašujeme, že naši práci na téma \It{\@titlecz} jsme vypracovali samostatně pod vedením
    % {\@mentorstatement} a~s~použitím odborné literatury a~dalších informačních zdrojů, které jsou
    % všechny citovány v~práci a~uvedeny v~seznamu literatury na~konci práce.
    %
    % Dále prohlašujeme, že tištěná i~elektronická verze práce SOČ jsou shodné a~nemáme závažný důvod
    % proti zpřístupňování této práce v~souladu se~zákonem č.~121/2000~Sb., o~právu autorském,
    % o~právech souvisejících s~právem autorským a~změně některých zákonů (autorský zákon) v~platném
    % změní.

    Prohlašujeme, že jsme naši práci SOČ vypracovali samostatně a použili jsme pouze prameny a
    literaturu uvedené v seznamu bibliografických záznamů.

    Bereme na vědomí, že nejpozději odevzdáním slovesné vědecké práce do veřejné soutěže Středoškolská
    odborná činnost, stejně jako odevzdáním jejích příloh a dalších připojených děl, např.
    audiovizuálních, fotografických, výtvarných, architektonických apod. (dále jen „soutěžní dílo“),
    dochází ke zveřejnění díla podle § 4 odst. 1 zákona č. 121/2000 Sb., autorského zákona, ve znění
    pozdějších předpisů (dále jen „autorský zákon“). Totéž platí pro pozdější odevzdání doplněného,
    změněného, upraveného nebo opraveného díla.

    Bereme na vědomí, že zveřejněním díla, jehož součástí je vynález, se tento vynález stává součástí
    stavu techniky podle § 5 odst. 1, 2 zákona č. 527/1990 Sb., o vynálezech, průmyslových vzorech a
    zlepšovacích návrzích, ve znění pozdějších předpisů (dále jen „patentový zákon“), což zakládá
    překážku pro udělení patentu podle § 3 odst. 1 patentového zákona.

    Bereme na vědomí, že vyhlašovatel soutěže je podle § 61 odst. 1 autorského zákona per analogiam
    oprávněn užít soutěžní dílo pro účely zajištění průběhu soutěže, zejména k zajištění
    transparentnosti soutěže a veřejnosti obhajob soutěžních prací. V odůvodněném rozsahu je tedy
    vyhlašovatel po dobu účasti autorů v soutěži oprávněn zejména:

    \begin{itemize}
        \item zhotovovat rozmnoženiny díla, je-li to nezbytné k seznámení účastníků soutěže, porotců
        nebo veřejnosti se soutěžní prací;
        \item zapůjčit originál nebo rozmnoženinu díla účastníkům soutěže, porotcům nebo veřejnosti.
        Přitom dbá na bezpečné nakládání s dílem;
        \item vystavovat originál nebo rozmnoženinu díla v průběhu soutěžních přehlídek a doprovodných
        akcí;
        \item sdělovat dílo veřejnosti v nehmotné podobě, a to především počítačovou nebo obdobnou sítí.
    \end{itemize}

    Dále prohlašujeme, že při tvorbě této práce jsme použili následující nástroje generativního
    modelu AI:

    \begin{itemize}
        \item ChatGPT (\url{https://chatgpt.com/}) -- kontrola gramatiky, úprava textu.
        \item Google Gemini (\url{https://gemini.google.com/}) -- kontrola gramatiky, úprava textu.
    \end{itemize}

    Po použití těchto nástrojů jsme provedli kontrolu obsahu a přebíráme za něj plnou zodpovědnost.

    \vspace{10mm}

    \noindent \parbox{\textwidth}{
        \noindent {#1} dne \today: \rule{8cm}{1pt}
    }

    \newpage
    \pagestyle{plain}
}

\newcommand{\makethanks}[1]{
    % \pagestyle{empty}
    % ~
    % \vfill
    \section*{Poděkování}
    {#1}

    % \vspace{\baselineskip}
    % \noindent Tato práce byla provedena za finanční podpory Jihomoravského kraje.

    \newpage
    \pagestyle{plain}
}



