# Использование avx инструкций для ускорения вычислений

## Описание 

Цель проекта - посмотреть, как использование avx инструкций влияет на производительность программы. В качестве алгоритма для применения intrinsic'ов было выбрано построения множества Мандельброта, так как с точки зрения вычисления цвета каждой точки это достаточно затратный алгоритм и на нем будет заметно видна разница в производительности. Чтобы не было так скучно, все это визуализируется с помощью SFML. Было написано 3 программы:
- Стандартная программа без использования каких-либо оптимизаций. Запуск с оптимизацией -O3.
- Программа, в которой происходит эмуляция avx инструкций через массивы такой же длины. Запуск с оптимизациями -O0 и -O3.
- Полноценное использование avx инструкций. Запуск с оптимизацией -O3.

Получившееся изображение:

![mandelbrot]()
 
Для замера времени используется код на [ассемблере](/Src/GetTimeStampCounter.s), который использует инструкцию `rdtsc` для подсчета количество циклов процессора, потраченных на пересчет. Так как для сравнения производительности достаточно поделить друг на друга результаты при разных запусках переводить это время в секунды не имеет смысла - такого представления достаточно. Когда выполняются замеры, функцию вывода изображения отключается, чтобы меньше приходилось ждать - выполняется много замеров, а затем высчитывается среднее время для большей точности. 

## Установка


## Первая программа 

[Код](/Src/NoAvx.cpp) 1000 раз пересчитывает Мандельброта, а затем усредняет время. Итоговое время:
-O0: 670635944
-O3: 379457887

## Третья программа

Рассмотрим сначала [третью программу](/Src/Avx.cpp), тогда идею второй будет легче понять. Мой процессор поддерживает avx и avx2, то есть вектора размером в 256 бит или 32 байта. 

Относительно первой программы меняется только часть пересчета цвета пикселей. Основная идея: наш цикл пробегается по координатам y, внутри по координатам x. Цвет какого-то конкретного x никак не зависит от результатов для соседних координат, поэтому их можно считать параллельно. Для этого используем регистры ymm, которые способны хранить 32 байта и совершать операции параллельно. Так как я использую тип данных float, то ожидаемое ускорение - в 8 раз (32 байта / sizeof(float), где sizeof(float) = 4 байта). Алгоритм по шагам:
- Выделяю соседние 8 пикселей [x, x + 1, x + 2, ..., x + 7], записываем их координаты в переменную типа __m256
- Воспринимая каждую координату как float и используя то, что операции с ymm регистрами выполняются параллельно над каждым участком из X байт (X зависит от типа операции) мы можем параллельно пересчитывать 8 точек.
- Цикл продолжается, пока последняя точка из выбранного множества не выйдет за круг заданного радиуса(часть алгоритма построения множества Мандельброта). Так как это построение множества Мандельброта, можно не следить за тем, что какая-то точка может сначала выйти из круга, а затем вернуться, так как если точка вышла, то дальше она только отдаляется. Этот факт облегчает построения алгоритма. При этом для каждой точки надо зафиксировать за сколько итераций она вышла за грани - этот момент рассмотрим поподробнее после перечисления действий алгоритма.
- После пересчета регистры mm256 можно воспринимать как массивы данных, по которым будем итерироваться. Создаем указатели нужных типов и указываем ими на начало памяти, где лежат наши данные. Далее в простом цикле заполняем цвета для пикселей.

Теперь рассмотрим подробнее момент, как фиксируется количество итераций, после которых точка вылетела за границу круга. Рассмотрим подробно следующие строчки:

```
__m256 cmpRadius = _mm256_cmp_ps(radiusSquare, maxRadiusSquare, _CMP_LT_OQ);
numberOfIterations = _mm256_sub_epi32(numberOfIterations, _mm256_castps_si256(cmpRadius));
```

Что лежит в cmpRadius? Каждый полученный радиус сравнивается с максимальным и в каждой из 8 ячеек появляется одно из двух чисел:
- 0, если точка вышла за границы
- 0xFFFFFFFF, если точка лежит в границах

Заметим, что 0xFFFFFFFF можно интерпретировать, как -1. Тогда для каждой точки из уже подсчитанного количества итераций для нее достаточно вычесть cmpRadius, которое интерпретируется, как int значения(это делается с помощью инструкции `_mm256_casrps_si256(cmpRadius)`). Если точка внутри круга - вычитается (-1), то есть число итераций увеличивается на одну. Если точка вне круга - вычитается ноль, то есть ничего не происходит. Тогда количество итераций для каждой точки считается верно. 

Измерение времени с оптимизацией -O3:
- 51913873

Это в $\frac{379457887}{51913873} = 7.31$ раз быстрее, чем без avx инструкций. Почему выигрыш оказался не в 8 раз, а всего в 7.27? Мои предположения:
- Дополнительные инструкции - теперь необходимо считать mask, чтобы определить, нужно ли дальше продолжать цикл.
- Цикл продолжается, пока самая последняя точка не выйдет за границу круга. Это может оказываться довольно сильное влияние, так как значительную часть картинки занимают точки, которые бесконечно циклятся и ограничиваются сверху максимальным числом итераций.
- В конце цикла необходимо достать по отдельности различные значение из ymm регистра, это довольно трудоемкая задача. 
- Запись полученных значений в массив пикселей никак не ускорялось. 

## Вторая программа

Теперь понять [вторую программу](/Src/NoAvxArrays.cpp) несложно - попытаемся сделать то же самое, что и с avx инструкциями, только эмулировать это с помощью массива, а потом проверить, сможет ли компилятор с флагами компиляции -O3 -mavx2 оптимизировать полученный код. Функции в данном примере названы по подобию с intrinsics-ами. 

