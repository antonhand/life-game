/*!
\file
\brief Заголовочный файл функций обработки команд

Данный файл содержит в себе определения функций, необходимых для обработки различных команд, поступающих от клиента, и для процесса моделирования игры
*/

#ifndef LIFEOP_H_INCLUDED
#define LIFEOP_H_INCLUDED
#include "lifefld.h"

enum
{
    RIGHTS = 0666,///< Права доступа
    NSEM = 6,     ///< Количество семафоров
    CMDSIZE = 32, ///< Максимальная длина команды
    DATA_SIZE = 2, ///< Длина тела сообщения в очереди сообщений
    OUT_SIZE = 70
};

/// Набор возможных ошибок
enum
{
    LF_ER_CMD = -1,     ///< Команда не найдена
    LF_ER_OUTOFFLD = -2,///< Заданные координаты оказались за пределами поля
    LF_ER_WRNUMARG = -3,///< Неверное количество аргументов операции
    LF_ER_BADARG = -4   ///< Недопустимые аргументы операции

};



/// Типы команд
enum
{
    LF_ADD = 1,   ///< Добавить клетку
    LF_CLR = 2,   ///< Очистить поле
    LF_START = 3, ///< Начать моделирование
    LF_STOP = 4,  ///< Остановить моделирование
    LF_SNAP = 5,  ///< Показать состояние поля
    LF_QUIT = 6   ///< Завершить выполнение программы
};

/// Параметры игры
enum
{
    LF_MINNEED = 2, ///< Минимальное необходимое количество соседей для продолжения жизни
    LF_MAXNEED = 3, ///< Максимальное допустимое количество соседей для продолжения жизни
    LF_BORN = 3     ///< Необходимое количество соседей для появления новой клетки
};

/// Буфер сообщения
struct msgb{
    long msgtype;        ///< Тип сообщения
    int data[DATA_SIZE]; ///< Тело сообщения
};

char outbuf[OUT_SIZE];

/*!
Преобразует команду, поступившую в виде строки, и отправляет её рабочим процессам
\param[in] cmd Строка с командой
\param[in] fldp Параметры поля
\param[in] proc Параметры процесса
\param[in] msgid Массив дескрипторов очередей сообщений к процессам
\param[in] semid Дескриптор массива семафоров
\return 0 в случае успешной обработки, номер ошибки в случае поступления некорректной команды
*/
int
processcmd(char *cmd, struct fld_param *fldp, int *msgid, int semid);

/*!
Очищает участок поля процесса от «живых» клеток
\param[in] fld Указатель на поле
\param[in] borders Указатель на структуру с границами участка поля процесса
\param[in] proc Параметры процесса
*/
void
clearfld(int **fld, struct border *borders, struct proc_param *proc);

/*!
Запускает и производит процесс моделирования игры
\param[in] P Номер поколения, до которого производить моделирование
\param[in] mode Режим запуска
\param[in] fld Указатель на поле
\param[in] borders Указатель на структуру с границами участка поля процесса
\param[in] fldp Параметры поля
\param[in] proc Параметры процесса
\param[in] semid Дескриптор массива семафоров
\param[in] num Номер процесса
\param[in] msgid Массив дескрипторов очередей сообщений к процессам
\return 0 в случае успешной обработки, LF_QUIT, в случае получения команды о выходе из программы во время работы функции
\details Вызывается в случае поступления команды «start {P}»/«старт {P}» от клиента.

В случае, если P < 0 выполняет моделирование до команды остановки либо до команды завершения работы программы.

Имеет 2 режима работы (mode): 0 - скрытый режим (не отображает состояние поля до поступления соответствующей команды) 1 - режим визуализации изменения состояния клеточной поверхности по ходу моделирования (для наглядности при этом процесс моделирования искусственно тормозится).
Режим визуализации включается добавлением к команде «start {P}»/«старт {P}» флага «-rt»/«-рв».

После моделирования каждого поколения проверяет, не поступила ли новая команда. В случае поступления команды сначала выполняет её, а затем приступает к моделированию следующего поколения.
*/
int
start(
    int P,
    int mode,
    int **fld,
    struct border *borders,
    struct fld_param *fldp,
    struct proc_param *proc,
    int semid,
    int num,
    int *msgid);

/*!
Выводит на экран текущее состояние поля
\param[in] fld Указатель на поле
\param[in] mode Режим отображения
\param[in] fldp Параметры поля
\param[in] proc Параметры процесса
\param[in] semid Дескриптор массива семафоров
\param[in] num Номер процесса
\details Вызывается в случае поступления команды «snapshot»/«состояние» от клиента.

Имеет 2 режима отображения: 0 - отображение живых и не живых клеток, 1 - отображение номера отвечающего за каждую клетку процесса.
Режим отображения номеров процессов включается добавлением к команде «snapshot»/«состояние» флага «-prcs»/«-прцс».
*/
void
snap(int **fld, int mode, struct fld_param *fldp, struct proc_param *proc, int semid, int num);

#endif // LIFEOP_H_INCLUDED