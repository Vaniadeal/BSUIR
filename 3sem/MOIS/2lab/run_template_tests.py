#!/usr/bin/env python3
"""
Автоматизированный стенд тестирования sc-шаблонов лабораторной работы №2 по МОИС / OSTIS.
Выполняет изоморфный поиск по онтологии детского сада в sc-памяти (ws://localhost:8090).
"""

import sys
import time
from sc_client import client
from sc_client.models import ScAddr, ScIdtfResolveParams

def get_identifier(addr: ScAddr) -> str:
    """Получить системный или основной идентификатор элемента"""
    try:
        from sc_client.constants import sc_types
        from sc_client.models import ScTemplate
        # Попробуем найти main_idtf
        nrel_main_idtf = client.resolve_keynodes(ScIdtfResolveParams(idtf='nrel_main_idtf', is_type=False))[0]
        templ = ScTemplate()
        templ.triple_with_relation(
            addr,
            sc_types.EDGE_D_COMMON_VAR,
            sc_types.LINK_VAR,
            sc_types.EDGE_ACCESS_VAR_POS_PERM,
            nrel_main_idtf
        )
        matches = client.template_search(templ)
        if matches:
            link_addr = matches[0].get(2)
            content = client.get_link_content(link_addr)[0].data
            if content:
                return str(content)
    except Exception:
        pass
    return f"addr_{addr.value}"

def run_tests():
    print("=" * 80)
    print("      ТЕСТИРОВАНИЕ SC-ШАБЛОНОВ (ЛАБОРАТОРНАЯ РАБОТА №2 - OSTIS 0.10.0)")
    print("=" * 80)
    print("Подключение к sc-server ws://localhost:8090 ...")
    
    try:
        client.connect("ws://localhost:8090")
    except Exception as e:
        print(f"[ОШИБКА] Не удалось подключиться к sc-server: {e}")
        sys.exit(1)
        
    print("[УСПЕХ] Соединение с sc-server установлено.\n")

    test_cases = [
        {
            "id": "Шаблон 1 (Положительный)",
            "name": "template_search_children",
            "title": "Поиск экземпляров класса (все дети детского сада)",
            "expected_count": 4,
            "type": "positive",
            "vars": ["_child"],
            "consts": ["concept_child"]
        },
        {
            "id": "Шаблон 2 (Положительный)",
            "name": "template_search_child_visits_group",
            "title": "Поиск пар сущностей в бинарном отношении (ребенок - группа)",
            "expected_count": 4,
            "type": "positive",
            "vars": ["_child", "_group"],
            "consts": ["nrel_visits_group"]
        },
        {
            "id": "Шаблон 3 (Положительный)",
            "name": "template_search_child_attributes",
            "title": "Поиск сущности по атрибутам (дети с именем и возрастом)",
            "expected_count": 2,
            "type": "positive",
            "vars": ["_child", "_name", "_age"],
            "consts": ["concept_child", "nrel_has_name", "nrel_has_age"]
        },
        {
            "id": "Шаблон 4 (Положительный)",
            "name": "template_search_petya_neighborhood",
            "title": "Семантическая окрестность Пети (группа + родитель + медсестра)",
            "expected_count": 1,
            "type": "positive",
            "vars": ["_group", "_parent", "_nurse"],
            "consts": ["Petya", "nrel_visits_group", "nrel_has_parent", "nrel_treats"]
        },
        {
            "id": "Шаблон 4 (Отрицательный)",
            "name": "template_search_vanya_neighborhood",
            "title": "Окрестность Вани (проверка отсутствия: медсестра не назначена)",
            "expected_count": 0,
            "type": "negative",
            "vars": ["_group", "_parent", "_nurse"],
            "consts": ["Vanya", "nrel_visits_group", "nrel_has_parent", "nrel_treats"]
        },
        {
            "id": "Шаблон 5 (Положительный)",
            "name": "template_search_educator_in_solnyshko",
            "title": "Воспитатель группы Солнышко со стажем (составное условие)",
            "expected_count": 1,
            "type": "positive",
            "vars": ["_educator", "_experience"],
            "consts": ["concept_educator", "Solnyshko", "nrel_works_in_group", "nrel_has_experience"]
        },
        {
            "id": "Шаблон 5 (Отрицательный)",
            "name": "template_search_educator_in_zvezdochki",
            "title": "Воспитатель группы Звездочки со стажем (проверка отсутствия)",
            "expected_count": 0,
            "type": "negative",
            "vars": ["_educator", "_experience"],
            "consts": ["concept_educator", "Zvezdochki", "nrel_works_in_group", "nrel_has_experience"]
        }
    ]

    total_passed = 0

    for i, tc in enumerate(test_cases, 1):
        print(f"[{i}/7] {tc['id']}: {tc['title']}")
        print(f"      Идентификатор: {tc['name']}")
        print(f"      Фиксированные узлы (константы): {', '.join(tc['consts'])}")
        print(f"      Искомые узлы (переменные):      {', '.join(tc['vars'])}")
        
        t0 = time.time()
        t_addr = client.resolve_keynodes(ScIdtfResolveParams(idtf=tc['name'], is_type=False))[0]
        
        if not t_addr.is_valid():
            print(f"      [СТАТУС]: ПРОВАЛ -> Шаблон {tc['name']} не найден в памяти!\n")
            continue

        results = client.template_search(t_addr)
        elapsed_ms = (time.time() - t0) * 1000
        count = len(results)

        passed = (count == tc['expected_count'])
        if passed:
            status_str = f"ВЫПОЛНЕН (Найдено: {count}, Ожидалось: {tc['expected_count']}) [{elapsed_ms:.2f} ms]"
            total_passed += 1
            print(f"      [РЕЗУЛЬТАТ]: {status_str}")
        else:
            status_str = f"ОШИБКА (Найдено: {count}, Ожидалось: {tc['expected_count']})"
            print(f"      [РЕЗУЛЬТАТ]: {status_str}")

        if count > 0:
            for idx, item in enumerate(results, 1):
                details = []
                for var in tc['vars']:
                    try:
                        val_addr = item.get(var)
                        details.append(f"{var}={val_addr.value}")
                    except Exception:
                        pass
                print(f"         Находка #{idx}: {', '.join(details)}")
        else:
            if tc['type'] == 'negative':
                print("         [OK] Конструкция в базе знаний отсутствует (пустое множество совпадений подтверждено).")
            else:
                print("         [ВНИМАНИЕ] Совпадений не обнаружено.")

        print("-" * 80)

    client.disconnect()
    print("\n" + "=" * 80)
    print(f"ИТОГ ТЕСТИРОВАНИЯ: Успешно пройдено {total_passed} из {len(test_cases)} тестов.")
    if total_passed == len(test_cases):
        print("ВСЕ 5 ПОЛОЖИТЕЛЬНЫХ И 2 ОТРИЦАТЕЛЬНЫХ ТЕСТА ПРОЙДЕНЫ НА 100%!")
    print("=" * 80)

if __name__ == "__main__":
    run_tests()
