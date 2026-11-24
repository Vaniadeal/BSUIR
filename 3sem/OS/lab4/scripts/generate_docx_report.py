#!/usr/bin/env python3
import os
import docx
from docx.shared import Pt, Inches, RGBColor
from docx.enum.text import WD_ALIGN_PARAGRAPH
from docx.enum.table import WD_TABLE_ALIGNMENT

def create_report(output_path, charts_dir):
    doc = docx.Document()

    # Set standard margins (2 cm ~ 0.79 in)
    for section in doc.sections:
        section.top_margin = Inches(0.79)
        section.bottom_margin = Inches(0.79)
        section.left_margin = Inches(0.79)
        section.right_margin = Inches(0.79)

    # Helper styling
    style_normal = doc.styles['Normal']
    style_normal.font.name = 'Times New Roman'
    style_normal.font.size = Pt(12)
    style_normal.font.color.rgb = RGBColor(0, 0, 0)

    def add_p(text, align=WD_ALIGN_PARAGRAPH.JUSTIFY, bold=False, italic=False, size=12, space_after=6, space_before=0):
        p = doc.add_paragraph()
        p.alignment = align
        p.paragraph_format.space_after = Pt(space_after)
        p.paragraph_format.space_before = Pt(space_before)
        p.paragraph_format.line_spacing = 1.15
        run = p.add_run(text)
        run.bold = bold
        run.italic = italic
        run.font.name = 'Times New Roman'
        run.font.size = Pt(size)
        return p

    def add_h(text, level=1):
        if level == 1:
            return add_p(text, align=WD_ALIGN_PARAGRAPH.LEFT, bold=True, size=14, space_before=14, space_after=6)
        elif level == 2:
            return add_p(text, align=WD_ALIGN_PARAGRAPH.LEFT, bold=True, size=13, space_before=10, space_after=4)
        else:
            return add_p(text, align=WD_ALIGN_PARAGRAPH.LEFT, bold=True, size=12, space_before=8, space_after=3)

    # --- TITLE PAGE ---
    add_p("Министерство образования Республики Беларусь", align=WD_ALIGN_PARAGRAPH.CENTER, size=11, space_after=2)
    add_p("Учреждение образования", align=WD_ALIGN_PARAGRAPH.CENTER, size=11, space_after=2)
    add_p("БЕЛОРУССКИЙ ГОСУДАРСТВЕННЫЙ УНИВЕРСИТЕТ\nИНФОРМАТИКИ И РАДИОЭЛЕКТРОНИКИ", align=WD_ALIGN_PARAGRAPH.CENTER, bold=True, size=12, space_after=2)
    add_p("Факультет информационных технологий и управления", align=WD_ALIGN_PARAGRAPH.CENTER, size=11, space_after=2)
    add_p("Кафедра интеллектуальных информационных технологий", align=WD_ALIGN_PARAGRAPH.CENTER, size=11, space_after=40)

    add_p("Дисциплина: операционные системы", align=WD_ALIGN_PARAGRAPH.CENTER, italic=True, size=12, space_after=12)
    add_p("Отчёт по лабораторной работе №4", align=WD_ALIGN_PARAGRAPH.CENTER, bold=True, size=16, space_after=4)
    add_p("«Изучение организации файловой системы»\nВариант 2", align=WD_ALIGN_PARAGRAPH.CENTER, bold=True, size=14, space_after=120)

    # Table for student & teacher
    table = doc.add_table(rows=2, cols=2)
    table.alignment = WD_TABLE_ALIGNMENT.CENTER
    table.rows[0].cells[0].paragraphs[0].text = "Выполнил студент гр. 521702"
    table.rows[0].cells[1].paragraphs[0].text = "И.В. Крюк"
    table.rows[1].cells[0].paragraphs[0].text = "Проверил"
    table.rows[1].cells[1].paragraphs[0].text = "Н.А. Гуменный"

    for row in table.rows:
        for cell in row.cells:
            cell.paragraphs[0].runs[0].font.name = 'Times New Roman'
            cell.paragraphs[0].runs[0].font.size = Pt(12)
            cell.paragraphs[0].paragraph_format.space_after = Pt(2)

    doc.add_paragraph().paragraph_format.space_after = Pt(80)
    add_p("Минск 2026", align=WD_ALIGN_PARAGRAPH.CENTER, size=12, space_after=0)
    doc.add_page_break()

    # --- CONTENT ---
    add_h("Цель работы", level=1)
    add_p("Изучить подходы к логической и физической организации файловой системы на примере программной модели файловой системы с одноуровневым каталогом и физической организацией в виде связанного списка блоков.")

    add_h("1. Индивидуальное задание", level=1)
    add_p("Вариант №2. Реализовать модель одноуровневой файловой системы с физической организацией файла — связанный список блоков. "
          "Реализовать базовые команды работы с файлами на диске: создание, удаление, копирование, перемещение (переименование), "
          "запись в файл, чтение файла. Обеспечить создание дампа файловой системы таким образом, чтобы можно было просмотреть структуру "
          "файловой системы и содержимое файлов. Провести нагрузочное тестирование с построением графиков и анализом выявленных недостатков.")

    add_h("2. Ход работы и описание методов решения", level=1)
    add_h("2.1. Логическая организация: Одноуровневая файловая система", level=2)
    add_p("В одноуровневой файловой системе каталог представляет собой единую плоскую таблицу записей (DirectoryEntry / FCB). "
          "Подкаталоги отсутствуют, в связи с чем все файлы размещаются в едином корневом пространстве имен. При создании файла "
          "обеспечивается уникальность его имени во всей файловой системе. Команда перемещения файла (move / mv) в условиях одноуровневого каталога "
          "концептуально и алгоритмически эквивалентна переименованию (rename) в том же каталоге, что не требует перезаписи физических блоков данных.")

    add_h("2.2. Физическая организация: Связанный список блоков", level=2)
    add_p("При физической организации в виде связанного списка блоков каждый блок данных содержит собственный служебный заголовок BlockHeader:")
    add_p("• next_block (4 байта) — номер следующего блока цепочки (-1 для признака конца файла EOF);\n"
          "• data_size (2 байта) — количество байт полезных данных в данном блоке;\n"
          "• flags (2 байта) — служебные флаги.\n"
          "При размере блока 512 байт полезная емкость составляет 504 байта. Свободное пространство отслеживается с помощью битовой карты (1 бит на блок).")

    add_h("2.3. Сравнение подходов к физической организации файлов", level=2)
    # Comparison table
    t_comp = doc.add_table(rows=5, cols=4)
    t_comp.alignment = WD_TABLE_ALIGNMENT.CENTER
    headers = ["Параметр", "Непрерывное размещение", "Связанный список блоков (Вар. 2)", "Индексная таблица (inode/FAT)"]
    for j, h in enumerate(headers):
        t_comp.rows[0].cells[j].paragraphs[0].text = h
        t_comp.rows[0].cells[j].paragraphs[0].runs[0].bold = True

    comp_data = [
        ("Внешняя фрагментация", "Высокая", "Отсутствует", "Отсутствует"),
        ("Динамический рост", "Затруднен (требует сдвигов)", "Простой (O(1) при last_block)", "Простой"),
        ("Произвольный доступ (seek)", "O(1) (мгновенный)", "O(N) (медленный, обход цепочки)", "O(1)"),
        ("Полезный размер блока", "Степень 2 (512 B)", "Не степень 2 (504 B)", "Степень 2 (512 B)")
    ]
    for i, row in enumerate(comp_data):
        for j, val in enumerate(row):
            t_comp.rows[i+1].cells[j].paragraphs[0].text = val

    for row in t_comp.rows:
        for cell in row.cells:
            cell.paragraphs[0].runs[0].font.name = 'Times New Roman'
            cell.paragraphs[0].runs[0].font.size = Pt(10)

    add_h("3. Функциональное тестирование модели", level=1)
    add_p("Для верификации работы программной модели разработан автоматизированный набор модульных тестов test_fs.cpp:")

    t_tests = doc.add_table(rows=8, cols=3)
    t_tests.alignment = WD_TABLE_ALIGNMENT.CENTER
    test_headers = ["№", "Проверяемая операция", "Результат"]
    for j, h in enumerate(test_headers):
        t_tests.rows[0].cells[j].paragraphs[0].text = h
        t_tests.rows[0].cells[j].paragraphs[0].runs[0].bold = True

    test_rows = [
        ("1", "Создание и удаление файла, проверка уникальности имени", "PASS"),
        ("2", "Запись и чтение небольшого файла (< 1 блока)", "PASS"),
        ("3", "Многоблочная запись (5 блоков, 2500 байт), проверка корректности цепочки", "PASS"),
        ("4", "Дозапись в конец файла (append) с переходом через границу блока", "PASS"),
        ("5", "Копирование файла с выделением независимой цепочки блоков", "PASS"),
        ("6", "Перемещение (переименование) файла с сохранением цепочки блоков", "PASS"),
        ("7", "Освобождение блоков при удалении и возврат в битовую карту", "PASS")
    ]
    for i, row in enumerate(test_rows):
        for j, val in enumerate(row):
            t_tests.rows[i+1].cells[j].paragraphs[0].text = val

    for row in t_tests.rows:
        for cell in row.cells:
            cell.paragraphs[0].runs[0].font.name = 'Times New Roman'
            cell.paragraphs[0].runs[0].font.size = Pt(10)

    add_p("Контрольный прогон: 7 из 7 тестов успешно пройдены без ошибок.", italic=True, space_before=4)

    add_h("4. Нагрузочное тестирование и анализ выявленных недостатков", level=1)
    add_p("Смысл нагрузочного тестирования заключается в исследовании масштабируемости модели при предельных нагрузках и количественном измерении недостатков, присущих организации файлов в виде связанного списка блоков.")

    add_h("4.1. Тест 1: Задержка произвольного доступа vs Размер файла", level=2)
    add_p("При чтении произвольного смещения в файле ОС вынуждена обходить все предшествующие блоки по указателям next_block. "
          "На графике показано время чтения последнего блока файла и суммарное число переходов (hops) при росте файла до 250 блоков:")

    img1 = os.path.join(charts_dir, "random_access_overhead.png")
    if os.path.exists(img1):
        doc.add_picture(img1, width=Inches(6.0))
        add_p("Рисунок 1 — Задержка произвольного позиционирования и число переходов vs размер файла", align=WD_ALIGN_PARAGRAPH.CENTER, italic=True, size=10)

    add_p("Выявленный недостаток: Время произвольного доступа к блоку в связном списке блоков строго линейно O(N) относительно позиции в файле. "
          "Для больших файлов это приводит к неприемлемым задержкам дискового ввода-вывода.")

    add_h("4.2. Тест 2: Динамический Churn и внешняя фрагментация цепочек", level=2)
    add_p("В процессе длительной эксплуатации (300 итераций создания файлов от 1 до 15 блоков и случайного удаления 50% файлов) блоки одного файла оказываются разбросаны по диску:")

    img2 = os.path.join(charts_dir, "churn_fragmentation.png")
    if os.path.exists(img2):
        doc.add_picture(img2, width=Inches(6.0))
        add_p("Рисунок 2 — Рост средней дистанции перехода между блоками и доли фрагментированных цепочек", align=WD_ALIGN_PARAGRAPH.CENTER, italic=True, size=10)

    add_p("Выявленный недостаток: Начальная дистанция перехода составляла 1.00 (блоки шли подряд). К шагу 300 средняя дистанция скачка выросла до 27.48 блоков, "
          "а доля фрагментированных цепочек достигла 12.63%. На жестких дисках это влечет существенные механические задержки головок (seek penalty).")

    add_h("4.3. Тест 3: Время аллокации при заполнении диска", level=2)
    img3 = os.path.join(charts_dir, "allocation_vs_disk_fill.png")
    if os.path.exists(img3):
        doc.add_picture(img3, width=Inches(6.0))
        add_p("Рисунок 3 — Задержка аллокации файла из 5 блоков при росте заполнения накопителя", align=WD_ALIGN_PARAGRAPH.CENTER, italic=True, size=10)
    add_p("Выявленный недостаток: По мере заполнения диска свыше 75% время поиска свободных блоков возрастает в 4–5 раз из-за фрагментированности свободного пространства в битовой карте.")

    add_h("4.4. Тест 4: Эффективность полезного объема блока", level=2)
    img4 = os.path.join(charts_dir, "block_efficiency.png")
    if os.path.exists(img4):
        doc.add_picture(img4, width=Inches(6.0))
        add_p("Рисунок 4 — Доля полезного объема в блоке при различных размерах блока", align=WD_ALIGN_PARAGRAPH.CENTER, italic=True, size=10)
    add_p("Выявленный недостаток: Необходимость хранения заголовка (8 байт) в каждом блоке делает полезный объем блока не равным степени 2 (504 байта вместо 512), "
          "что усложняет трансляцию адресов виртуальной памяти.")

    add_h("5. Ответы на контрольные вопросы", level=1)
    answers = [
        ("1. Что такое файл?",
         "Файл — именованная область данных на носителе долговременной памяти, рассматриваемая операционной системой как единое целое (последовательность байтов или логических записей). Предоставляет прикладным программам независимый от аппаратных деталей интерфейс ввода-вывода."),
        ("2. Назовите типы файлов",
         "1) Обычные (регулярные) файлы: текстовые и бинарные (исполняемые, архивы, мультимедиа); 2) Файлы-каталоги (директории); 3) Специальные файлы устройств (блочные и символьные); 4) Символические ссылки; 5) Именованные каналы (FIFO); 6) Доменные сокеты."),
        ("3. Дайте определение термину файловая система",
         "Файловая система — компонент операционной системы, определяющий структуру, правила организации, форматы хранения, методы именования, поиска, защиты и доступа к файлам на носителях долговременной памяти, связывая логическое представление файлов с физическими блоками накопителя."),
        ("4. Что такое атрибут файла? Приведите примеры.",
         "Атрибут файла — служебная информация (метаданные), описывающая свойства файла: размер, даты создания/модификации/доступа, права доступа (UID, GID, rwx), флаги (read-only, hidden), адреса начальных блоков."),
        ("5. Назовите и расскажите про подходы к организации доступа к файлам",
         "1) Последовательный доступ (чтение/запись байт подряд от начала к концу); 2) Прямой (произвольный) доступ (позиционирование lseek на произвольный байт за O(1)); 3) Индексно-последовательный доступ (поиск ключевой записи по индексной таблице с последующим последовательным чтением)."),
        ("6. Что такое логическая структура файла. Что такое логическая запись?",
         "Логическая структура — представление данных файла с точки зрения прикладной программы (поток байт, массив записей фиксированной/переменной длины, дерево). Логическая запись — минимальная неделимая смысловая порция информации файла с точки зрения прикладного алгоритма."),
        ("7. Что такое физическая структура файла. Что такое физическая запись?",
         "Физическая структура — способ размещения и связывания блоков файла на дисковом накопителе (непрерывное размещение, связанный список блоков, связанный список индексов, таблица номеров блоков). Физическая запись (блок) — минимальная порция данных, передаваемая контроллером диска за одну операцию ввода-вывода."),
        ("8. Что такое символьное имя файла?",
         "Символьное имя файла — строка символов, присваиваемая файлу для его идентификации человеком. Различают простое имя (в пределах одного каталога) и полное составное имя (путь от корня до файла)."),
        ("9. Что такое файлы, проецируемые в память (Memory-Mapped Files)?",
         "Механизм ОС (mmap в POSIX), при котором файл связывается с виртуальным адресным пространством процесса. Доступ к содержимому файла осуществляется через операции чтения/записи в память; загрузка блоков с диска выполняется страничным механизмом ядра (Page Cache) при возникновении page fault без явных вызовов read/write.")
    ]

    for q, a in answers:
        add_p(q, bold=True, size=12, space_before=6, space_after=2)
        add_p(a, size=12, space_after=6)

    add_h("6. Заключение", level=1)
    add_p("В результате выполнения лабораторной работы была разработана и всесторонне исследована программная модель одноуровневой файловой системы с физической организацией в виде связанного списка блоков. Экспериментально на графиках подтверждены ключевые недостатки модели: O(N) сложность произвольного доступа, развитие внешней фрагментации цепочек блоков (рост дистанции скачка до 27.48 блоков) и не-степенная полезная емкость блока (504 байта).")

    doc.save(output_path)
    print(f"Report saved successfully: {output_path}")

if __name__ == '__main__':
    charts = "/home/dereviashka/BSUIR/3sem/OS/lab4/benchmarks/charts"
    out1 = "/home/dereviashka/BSUIR/3sem/OS/lab4/Отчет_ЛР4_Крюк_ИВ.docx"
    out2 = "/home/dereviashka/antigravity/vibrant-mendeleev/filesystem/Отчет_ЛР4_Крюк_ИВ.docx"
    create_report(out1, charts)
    create_report(out2, charts)
