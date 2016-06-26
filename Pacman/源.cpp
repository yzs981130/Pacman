/*
*  ���ߣ�Ҷ־��
* ������������
*  r/R/y/Y��Row���У�������
*  c/C/x/X��Column���У�������
*  ������±궼��[y][x]��[r][c]��˳��
*  ��ұ��0123
*
* ������ϵ��
*   0 1 2 3 4 5 6 7 8
* 0 +----------------> x
* 1 |
* 2 |
* 3 |
* 4 |
* 5 |
* 6 |
* 7 |
* 8 |
*   v y
*
* ����ʾ�������ʹ��
* #ifndef _BOTZONE_ONLINE
* ������Ԥ����ָ����������������ͱ�������
*
* ����ʾ��һ����ı��༭������֧�ֽ�������۵�����
* ���������Դ�����̫���߳������Կ��ǽ�����namespace�۵�
*/

#include <fstream>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <iostream>
#include <algorithm>
#include <string>
#include <cstring>
#include <stack>
#include <stdexcept>
#include "jsoncpp/json.h"
#include <stack>
#include <queue>
#include<sstream>

#define DEPTH 5
#define ShortestDangerDistance 5
#define FIELD_MAX_HEIGHT 20
#define FIELD_MAX_WIDTH 20
#define MAX_GENERATOR_COUNT 4 // ÿ������1
#define MAX_PLAYER_COUNT 4
#define MAX_TURN 100

// ��Ҳ����ѡ�� using namespace std; ���ǻ���Ⱦ�����ռ�
using std::string;
using std::swap;
using std::cin;
using std::cout;
using std::endl;
using std::getline;
using std::runtime_error;
using std::queue;
using std::stack;
using std::stringstream;

// ƽ̨�ṩ�ĳԶ�������߼��������
namespace Pacman
{
    const time_t seed = time(0);
    const int dx[] = {0, 1, 0, -1, 1, 1, -1, -1}, dy[] = {-1, 0, 1, 0, -1, 1, 1, -1};

    // ö�ٶ��壻ʹ��ö����Ȼ���˷ѿռ䣨sizeof(GridContentType) == 4�������Ǽ��������32λ������Ч�ʸ���

    // ÿ�����ӿ��ܱ仯�����ݣ�����á����߼��������
    enum GridContentType
    {
        empty = 0, // ��ʵ�����õ�
        player1 = 1, // 1�����
        player2 = 2, // 2�����
        player3 = 4, // 3�����
        player4 = 8, // 4�����
        playerMask = 1 | 2 | 4 | 8, // ���ڼ����û����ҵ�
        smallFruit = 16, // С����
        largeFruit = 32 // ����
    };

    // �����ID��ȡ��������ҵĶ�����λ
    GridContentType playerID2Mask[] = {player1, player2, player3, player4};
    string playerID2str[] = {"0", "1", "2", "3"};

    // ��ö��Ҳ��������Щ�����ˣ����ӻ�������
    template<typename T>
    inline T operator |=(T &a, const T &b)
    {
        return a = static_cast<T>(static_cast<int>(a) | static_cast<int>(b));
    }
    template<typename T>
    inline T operator |(const T &a, const T &b)
    {
        return static_cast<T>(static_cast<int>(a) | static_cast<int>(b));
    }
    template<typename T>
    inline T operator &=(T &a, const T &b)
    {
        return a = static_cast<T>(static_cast<int>(a) & static_cast<int>(b));
    }
    template<typename T>
    inline T operator &(const T &a, const T &b)
    {
        return static_cast<T>(static_cast<int>(a) & static_cast<int>(b));
    }
    template<typename T>
    inline T operator ++(T &a)
    {
        return a = static_cast<T>(static_cast<int>(a) + 1);
    }
    template<typename T>
    inline T operator ~(const T &a)
    {
        return static_cast<T>(~static_cast<int>(a));
    }

    // ÿ�����ӹ̶��Ķ���������á����߼��������
    enum GridStaticType
    {
        emptyWall = 0, // ��ʵ�����õ�
        wallNorth = 1, // ��ǽ����������ٵķ���
        wallEast = 2, // ��ǽ�����������ӵķ���
        wallSouth = 4, // ��ǽ�����������ӵķ���
        wallWest = 8, // ��ǽ����������ٵķ���
        generator = 16 // ���Ӳ�����
    };

    // ���ƶ�����ȡ����������赲�ŵ�ǽ�Ķ�����λ
    GridStaticType direction2OpposingWall[] = {wallNorth, wallEast, wallSouth, wallWest};

    // ���򣬿��Դ���dx��dy���飬ͬʱҲ������Ϊ��ҵĶ���
    enum Direction
    {
        stay = -1,
        up = 0,
        right = 1,
        down = 2,
        left = 3,
        // ������⼸��ֻ��Ϊ�˲��������򷽱㣬����ʵ���õ�
        ur = 4, // ����
        dr = 5, // ����
        dl = 6, // ����
        ul = 7 // ����
    };

    // �����ϴ�����������
    struct FieldProp
    {
        int row, col;
        FieldProp(int r = 0, int c = 0):row(r), col(c) {}//ȱʡ��������
    };

    // �����ϵ����
    struct Player: FieldProp
    {
        int strength;
        int powerUpLeft;
        bool dead;
    };

    // �غ��²����Ķ��ӵ�����
    struct NewFruits
    {
        FieldProp newFruits[MAX_GENERATOR_COUNT * 8];
        int newFruitCount;
    } newFruits[MAX_TURN];
    int newFruitsCount = 0;

    // ״̬ת�Ƽ�¼�ṹ
    struct TurnStateTransfer
    {
        enum StatusChange // �����
        {
            none = 0,
            ateSmall = 1,
            ateLarge = 2,
            powerUpCancel = 4,
            die = 8,
            error = 16
        };

        // ���ѡ���Ķ���
        Direction actions[MAX_PLAYER_COUNT];

        // �˻غϸ���ҵ�״̬�仯
        StatusChange change[MAX_PLAYER_COUNT];

        // �˻غϸ���ҵ������仯
        int strengthDelta[MAX_PLAYER_COUNT];
    };

    // ��Ϸ��Ҫ�߼������࣬��������������غ����㡢״̬ת�ƣ�ȫ��Ψһ
    class GameField
    {
    private:
        // Ϊ�˷��㣬��������Զ�����private��

        // ��¼ÿ�غϵı仯��ջ��
        TurnStateTransfer backtrack[MAX_TURN];

        // ��������Ƿ��Ѿ�����
        static bool constructed;

    public:
        // ���صĳ��Ϳ�
        int height, width;
        int generatorCount;
        int GENERATOR_INTERVAL, LARGE_FRUIT_DURATION, LARGE_FRUIT_ENHANCEMENT;

        // ���ظ��ӹ̶�������
        GridStaticType fieldStatic[FIELD_MAX_HEIGHT][FIELD_MAX_WIDTH];

        // ���ظ��ӻ�仯������
        GridContentType fieldContent[FIELD_MAX_HEIGHT][FIELD_MAX_WIDTH];
        int generatorTurnLeft; // ���ٻغϺ��������
        int aliveCount; // �ж�����Ҵ��
        int smallFruitCount;
        int turnID;
        FieldProp generators[MAX_GENERATOR_COUNT]; // ����Щ���Ӳ�����
        Player players[MAX_PLAYER_COUNT]; // ����Щ���

                                          // ���ѡ���Ķ���
        Direction actions[MAX_PLAYER_COUNT];

        // �ָ����ϴγ���״̬������һ·�ָ����ʼ��
        // �ָ�ʧ�ܣ�û��״̬�ɻָ�������false
        bool PopState()
        {
            if (turnID <= 0)
                return false;

            const TurnStateTransfer &bt = backtrack[--turnID];
            int i, _;

            // �������ָ�״̬

            for (_ = 0; _ < MAX_PLAYER_COUNT; _++)
            {
                Player &_p = players[_];
                GridContentType &content = fieldContent[_p.row][_p.col];
                TurnStateTransfer::StatusChange change = bt.change[_];

                if (!_p.dead)
                {
                    // 5. �󶹻غϻָ�
                    if (_p.powerUpLeft || change & TurnStateTransfer::powerUpCancel)
                        _p.powerUpLeft++;

                    // 4. �³�����
                    if (change & TurnStateTransfer::ateSmall)
                    {
                        content |= smallFruit;
                        smallFruitCount++;
                    }
                    else if (change & TurnStateTransfer::ateLarge)
                    {
                        content |= largeFruit;
                        _p.powerUpLeft -= LARGE_FRUIT_DURATION;
                    }
                }

                // 2. �������
                if (change & TurnStateTransfer::die)
                {
                    _p.dead = false;
                    aliveCount++;
                    content |= playerID2Mask[_];
                }

                // 1. ���λ�Ӱ
                if (!_p.dead && bt.actions[_] != stay)
                {
                    fieldContent[_p.row][_p.col] &= ~playerID2Mask[_];
                    _p.row = (_p.row - dy[bt.actions[_]] + height) % height;
                    _p.col = (_p.col - dx[bt.actions[_]] + width) % width;
                    fieldContent[_p.row][_p.col] |= playerID2Mask[_];
                }

                // 0. ���겻�Ϸ������
                if (change & TurnStateTransfer::error)
                {
                    _p.dead = false;
                    aliveCount++;
                    content |= playerID2Mask[_];
                }

                // *. �ָ�����
                if (!_p.dead)
                    _p.strength -= bt.strengthDelta[_];
            }

            // 3. �ջض���
            if (generatorTurnLeft == GENERATOR_INTERVAL)
            {
                generatorTurnLeft = 1;
                NewFruits &fruits = newFruits[--newFruitsCount];
                for (i = 0; i < fruits.newFruitCount; i++)
                {
                    fieldContent[fruits.newFruits[i].row][fruits.newFruits[i].col] &= ~smallFruit;
                    smallFruitCount--;
                }
            }
            else
                generatorTurnLeft++;

            return true;
        }

        // �ж�ָ�������ָ�������ƶ��ǲ��ǺϷ��ģ�û��ײǽ��
        inline bool ActionValid(int playerID, Direction &dir) const
        {
            if (dir == stay)
                return true;
            const Player &p = players[playerID];
            const GridStaticType &s = fieldStatic[p.row][p.col];
            return dir >= -1 && dir < 4 && !(s & direction2OpposingWall[dir]);
        }

        // ����actionsд����Ҷ�����������һ�غϾ��棬����¼֮ǰ���еĳ���״̬���ɹ��պ�ָ���
        // ���վֵĻ��ͷ���false
        bool NextTurn()
        {
            int _, i, j;

            TurnStateTransfer &bt = backtrack[turnID];
            memset(&bt, 0, sizeof(bt));

            // 0. ɱ�����Ϸ�����
            for (_ = 0; _ < MAX_PLAYER_COUNT; _++)
            {
                Player &p = players[_];
                if (!p.dead)
                {
                    Direction &action = actions[_];
                    if (action == stay)
                        continue;

                    if (!ActionValid(_, action))
                    {
                        bt.strengthDelta[_] += -p.strength;
                        bt.change[_] = TurnStateTransfer::error;
                        fieldContent[p.row][p.col] &= ~playerID2Mask[_];
                        p.strength = 0;
                        p.dead = true;
                        aliveCount--;
                    }
                    else
                    {
                        // �������Լ�ǿ��׳������ǲ���ǰ����
                        GridContentType target = fieldContent
                            [(p.row + dy[action] + height) % height]
                        [(p.col + dx[action] + width) % width];
                        if (target & playerMask)
                            for (i = 0; i < MAX_PLAYER_COUNT; i++)
                                if (target & playerID2Mask[i] && players[i].strength > p.strength)
                                    action = stay;
                    }
                }
            }

            // 1. λ�ñ仯
            for (_ = 0; _ < MAX_PLAYER_COUNT; _++)
            {
                Player &_p = players[_];
                if (_p.dead)
                    continue;

                bt.actions[_] = actions[_];

                if (actions[_] == stay)
                    continue;

                // �ƶ�
                fieldContent[_p.row][_p.col] &= ~playerID2Mask[_];
                _p.row = (_p.row + dy[actions[_]] + height) % height;
                _p.col = (_p.col + dx[actions[_]] + width) % width;
                fieldContent[_p.row][_p.col] |= playerID2Mask[_];
            }

            // 2. ��һ�Ź
            for (_ = 0; _ < MAX_PLAYER_COUNT; _++)
            {
                Player &_p = players[_];
                if (_p.dead)
                    continue;

                // �ж��Ƿ��������һ��
                int player, containedCount = 0;
                int containedPlayers[MAX_PLAYER_COUNT];
                for (player = 0; player < MAX_PLAYER_COUNT; player++)
                    if (fieldContent[_p.row][_p.col] & playerID2Mask[player])
                        containedPlayers[containedCount++] = player;

                if (containedCount > 1)
                {
                    // NAIVE
                    for (i = 0; i < containedCount; i++)
                        for (j = 0; j < containedCount - i - 1; j++)
                            if (players[containedPlayers[j]].strength < players[containedPlayers[j + 1]].strength)
                                swap(containedPlayers[j], containedPlayers[j + 1]);

                    int begin;
                    for (begin = 1; begin < containedCount; begin++)
                        if (players[containedPlayers[begin - 1]].strength > players[containedPlayers[begin]].strength)
                            break;

                    // ��Щ��ҽ��ᱻɱ��
                    int lootedStrength = 0;
                    for (i = begin; i < containedCount; i++)
                    {
                        int id = containedPlayers[i];
                        Player &p = players[id];

                        // �Ӹ���������
                        fieldContent[p.row][p.col] &= ~playerID2Mask[id];
                        p.dead = true;
                        int drop = p.strength / 2;
                        bt.strengthDelta[id] += -drop;
                        bt.change[id] |= TurnStateTransfer::die;
                        lootedStrength += drop;
                        p.strength -= drop;
                        aliveCount--;
                    }

                    // ������������
                    int inc = lootedStrength / begin;
                    for (i = 0; i < begin; i++)
                    {
                        int id = containedPlayers[i];
                        Player &p = players[id];
                        bt.strengthDelta[id] += inc;
                        p.strength += inc;
                    }
                }
            }

            // 3. ��������
            if (--generatorTurnLeft == 0)
            {
                generatorTurnLeft = GENERATOR_INTERVAL;
                NewFruits &fruits = newFruits[newFruitsCount++];
                fruits.newFruitCount = 0;
                for (i = 0; i < generatorCount; i++)
                    for (Direction d = up; d < 8; ++d)
                    {
                        // ȡ�࣬�������ر߽�
                        int r = (generators[i].row + dy[d] + height) % height, c = (generators[i].col + dx[d] + width) % width;
                        if (fieldStatic[r][c] & generator || fieldContent[r][c] & (smallFruit | largeFruit))
                            continue;
                        fieldContent[r][c] |= smallFruit;
                        fruits.newFruits[fruits.newFruitCount].row = r;
                        fruits.newFruits[fruits.newFruitCount++].col = c;
                        smallFruitCount++;
                    }
            }

            // 4. �Ե�����
            for (_ = 0; _ < MAX_PLAYER_COUNT; _++)
            {
                Player &_p = players[_];
                if (_p.dead)
                    continue;

                GridContentType &content = fieldContent[_p.row][_p.col];

                // ֻ���ڸ�����ֻ���Լ���ʱ����ܳԵ�����
                if (content & playerMask & ~playerID2Mask[_])
                    continue;

                if (content & smallFruit)
                {
                    content &= ~smallFruit;
                    _p.strength++;
                    bt.strengthDelta[_]++;
                    smallFruitCount--;
                    bt.change[_] |= TurnStateTransfer::ateSmall;
                }
                else if (content & largeFruit)
                {
                    content &= ~largeFruit;
                    if (_p.powerUpLeft == 0)
                    {
                        _p.strength += LARGE_FRUIT_ENHANCEMENT;
                        bt.strengthDelta[_] += LARGE_FRUIT_ENHANCEMENT;
                    }
                    _p.powerUpLeft += LARGE_FRUIT_DURATION;
                    bt.change[_] |= TurnStateTransfer::ateLarge;
                }
            }

            // 5. �󶹻غϼ���
            for (_ = 0; _ < MAX_PLAYER_COUNT; _++)
            {
                Player &_p = players[_];
                if (_p.dead)
                    continue;

                if (_p.powerUpLeft > 0 && --_p.powerUpLeft == 0)
                {
                    _p.strength -= LARGE_FRUIT_ENHANCEMENT;
                    bt.change[_] |= TurnStateTransfer::powerUpCancel;
                    bt.strengthDelta[_] += -LARGE_FRUIT_ENHANCEMENT;
                }
            }

            ++turnID;

            // �Ƿ�ֻʣһ�ˣ�
            if (aliveCount <= 1)
            {
                for (_ = 0; _ < MAX_PLAYER_COUNT; _++)
                    if (!players[_].dead)
                    {
                        bt.strengthDelta[_] += smallFruitCount;
                        players[_].strength += smallFruitCount;
                    }
                return false;
            }

            // �Ƿ�غϳ��ޣ�
            if (turnID >= 100)
                return false;

            return true;
        }

        // ��ȡ�������������룬���ص��Ի��ύƽ̨ʹ�ö����ԡ�
        // ����ڱ��ص��ԣ�����������Ŷ�ȡ������ָ�����ļ���Ϊ�����ļ���ʧ�ܺ���ѡ��ȴ��û�ֱ�����롣
        // ���ص���ʱ���Խ��ܶ����Ա������Windows�¿����� Ctrl-Z ��һ��������+�س�����ʾ���������������������ֻ����ܵ��м��ɡ�
        // localFileName ����ΪNULL
        // obtainedData ������Լ��ϻغϴ洢�����غ�ʹ�õ�����
        // obtainedGlobalData ������Լ��� Bot ����ǰ�洢������
        // ����ֵ���Լ��� playerID
        int ReadInput(const char *localFileName, string &obtainedData, string &obtainedGlobalData)
        {
            string str, chunk;
#ifdef _BOTZONE_ONLINE
            std::ios::sync_with_stdio(false); //��\\)
            getline(cin, str);
#else
            if (localFileName)
            {
                std::ifstream fin(localFileName);
                if (fin)
                    while (getline(fin, chunk) && chunk != "")
                        str += chunk;
                else
                    while (getline(cin, chunk) && chunk != "")
                        str += chunk;
            }
            else
                while (getline(cin, chunk) && chunk != "")
                    str += chunk;
#endif
            Json::Reader reader;
            Json::Value input;
            reader.parse(str, input);

            int len = input["requests"].size();

            // ��ȡ���ؾ�̬״��
            Json::Value field = input["requests"][(Json::Value::UInt) 0],
                staticField = field["static"], // ǽ��Ͳ�����
                contentField = field["content"]; // ���Ӻ����
            height = field["height"].asInt();
            width = field["width"].asInt();
            LARGE_FRUIT_DURATION = field["LARGE_FRUIT_DURATION"].asInt();
            LARGE_FRUIT_ENHANCEMENT = field["LARGE_FRUIT_ENHANCEMENT"].asInt();
            generatorTurnLeft = GENERATOR_INTERVAL = field["GENERATOR_INTERVAL"].asInt();

            PrepareInitialField(staticField, contentField);

            // ������ʷ�ָ�����
            for (int i = 1; i < len; i++)
            {
                Json::Value req = input["requests"][i];
                for (int _ = 0; _ < MAX_PLAYER_COUNT; _++)
                    if (!players[_].dead)
                        actions[_] = (Direction)req[playerID2str[_]]["action"].asInt();
                NextTurn();
            }

            obtainedData = input["data"].asString();
            obtainedGlobalData = input["globaldata"].asString();

            return field["id"].asInt();
        }

        // ���� static �� content ����׼�����صĳ�ʼ״��
        void PrepareInitialField(const Json::Value &staticField, const Json::Value &contentField)
        {
            int r, c, gid = 0;
            generatorCount = 0;
            aliveCount = 0;
            smallFruitCount = 0;
            generatorTurnLeft = GENERATOR_INTERVAL;
            for (r = 0; r < height; r++)
                for (c = 0; c < width; c++)
                {
                    GridContentType &content = fieldContent[r][c] = (GridContentType)contentField[r][c].asInt();
                    GridStaticType &s = fieldStatic[r][c] = (GridStaticType)staticField[r][c].asInt();
                    if (s & generator)
                    {
                        generators[gid].row = r;
                        generators[gid++].col = c;
                        generatorCount++;
                    }
                    if (content & smallFruit)
                        smallFruitCount++;
                    for (int _ = 0; _ < MAX_PLAYER_COUNT; _++)
                        if (content & playerID2Mask[_])
                        {
                            Player &p = players[_];
                            p.col = c;
                            p.row = r;
                            p.powerUpLeft = 0;
                            p.strength = 1;
                            p.dead = false;
                            aliveCount++;
                        }
                }
        }

        // ��ɾ��ߣ���������
        // action ��ʾ���غϵ��ƶ�����stay Ϊ���ƶ�
        // tauntText ��ʾ��Ҫ��������������������ַ�����������ʾ����Ļ�ϲ������κ����ã����ձ�ʾ������
        // data ��ʾ�Լ���洢����һ�غ�ʹ�õ����ݣ����ձ�ʾɾ��
        // globalData ��ʾ�Լ���洢���Ժ�ʹ�õ����ݣ��滻����������ݿ��Կ�Ծ�ʹ�ã���һֱ������� Bot �ϣ����ձ�ʾɾ��
        void WriteOutput(Direction action, string tauntText = "", string data = "", string globalData = "") const
        {
            Json::Value ret;
            ret["response"]["action"] = action;
            ret["response"]["tauntText"] = tauntText;
            ret["data"] = data;
            ret["globaldata"] = globalData;
            ret["debug"] = (Json::Int)seed;

#ifdef _BOTZONE_ONLINE
            Json::FastWriter writer; // ��������Ļ����þ��С���
#else
            Json::StyledWriter writer; // ���ص��������ÿ� > <
#endif
            cout << writer.write(ret) << endl;
        }

        // ������ʾ��ǰ��Ϸ״̬�������á�
        // �ύ��ƽ̨��ᱻ�Ż�����
        inline void DebugPrint() const
        {
#ifndef _BOTZONE_ONLINE
            printf("�غϺš�%d�����������%d��| ͼ�� ������[G] �����[0/1/2/3] ������[*] ��[o] С��[.]\n", turnID, aliveCount);
            for (int _ = 0; _ < MAX_PLAYER_COUNT; _++)
            {
                const Player &p = players[_];
                printf("[���%d(%d, %d)|����%d|�ӳ�ʣ��غ�%d|%s]\n",
                    _, p.row, p.col, p.strength, p.powerUpLeft, p.dead ? "����" : "���");
            }
            putchar(' ');
            putchar(' ');
            for (int c = 0; c < width; c++)
                printf("  %d ", c);
            putchar('\n');
            for (int r = 0; r < height; r++)
            {
                putchar(' ');
                putchar(' ');
                for (int c = 0; c < width; c++)
                {
                    putchar(' ');
                    printf((fieldStatic[r][c] & wallNorth) ? "---" : "   ");
                }
                printf("\n%d ", r);
                for (int c = 0; c < width; c++)
                {
                    putchar((fieldStatic[r][c] & wallWest) ? '|' : ' ');
                    putchar(' ');
                    int hasPlayer = -1;
                    for (int _ = 0; _ < MAX_PLAYER_COUNT; _++)
                        if (fieldContent[r][c] & playerID2Mask[_])
                            if (hasPlayer == -1)
                                hasPlayer = _;
                            else
                                hasPlayer = 4;
                    if (hasPlayer == 4)
                        putchar('*');
                    else if (hasPlayer != -1)
                        putchar('0' + hasPlayer);
                    else if (fieldStatic[r][c] & generator)
                        putchar('G');
                    else if (fieldContent[r][c] & playerMask)
                        putchar('*');
                    else if (fieldContent[r][c] & smallFruit)
                        putchar('.');
                    else if (fieldContent[r][c] & largeFruit)
                        putchar('o');
                    else
                        putchar(' ');
                    putchar(' ');
                }
                putchar((fieldStatic[r][width - 1] & wallEast) ? '|' : ' ');
                putchar('\n');
            }
            putchar(' ');
            putchar(' ');
            for (int c = 0; c < width; c++)
            {
                putchar(' ');
                printf((fieldStatic[height - 1][c] & wallSouth) ? "---" : "   ");
            }
            putchar('\n');
#endif
        }

        Json::Value SerializeCurrentTurnChange()
        {
            Json::Value result;
            TurnStateTransfer &bt = backtrack[turnID - 1];
            for (int _ = 0; _ < MAX_PLAYER_COUNT; _++)
            {
                result["actions"][_] = bt.actions[_];
                result["strengthDelta"][_] = bt.strengthDelta[_];
                result["change"][_] = bt.change[_];
            }
            return result;
        }

        // ��ʼ����Ϸ������
        GameField()
        {
            if (constructed)
                throw runtime_error("�벻Ҫ�ٴ��� GameField �����ˣ�����������ֻӦ����һ������");
            constructed = true;

            turnID = 0;
        }

        GameField(const GameField &b): GameField() {}
    };

    bool GameField::constructed = false;
}

// һЩ��������
namespace Helpers
{

    int actionScore[5] = {};
    int TempMaxValue = -99999, TempValue = 0;
    Pacman::Direction TempAction[5];
    Pacman::Direction MyDFSAct;
    Pacman::FieldProp BeginPosition;

    //����==��bfs��
    bool operator ==(const Pacman::FieldProp & a, const Pacman::FieldProp & b)
    {
        return a.col == b.col && a.row == b.row ? true : false;
    }

    inline int RandBetween(int a, int b)
    {
        if (a > b)
            swap(a, b);
        return rand() % (b - a) + a;
    }

    void RandomPlay(Pacman::GameField &gameField, int myID)
    {
        int count = 0, myAct = -1;
        while (true)
        {
            // ��ÿ�������������ĺϷ�����
            for (int i = 0; i < MAX_PLAYER_COUNT; i++)
            {
                if (gameField.players[i].dead)
                    continue;
                Pacman::Direction valid[5];
                int vCount = 0;
                for (Pacman::Direction d = Pacman::stay; d < 4; ++d)
                    if (gameField.ActionValid(i, d))
                        valid[vCount++] = d;
                gameField.actions[i] = valid[RandBetween(0, vCount)];
            }

            if (count == 0)
                myAct = gameField.actions[myID];

            // ����һ������仯
            // NextTurn����true��ʾ��Ϸû�н���
            bool hasNext = gameField.NextTurn();
            count++;

            if (!hasNext)
                break;
        }

        // �������
        int rank2player[] = {0, 1, 2, 3};
        for (int j = 0; j < MAX_PLAYER_COUNT; j++)
            for (int k = 0; k < MAX_PLAYER_COUNT - j - 1; k++)
                if (gameField.players[rank2player[k]].strength > gameField.players[rank2player[k + 1]].strength)
                    swap(rank2player[k], rank2player[k + 1]);

        int scorebase = 1;
        if (rank2player[0] == myID)
            actionScore[myAct + 1]++;
        else
            for (int j = 1; j < MAX_PLAYER_COUNT; j++)
            {
                if (gameField.players[rank2player[j - 1]].strength < gameField.players[rank2player[j]].strength)
                    scorebase = j + 1;
                if (rank2player[j] == myID)
                {
                    actionScore[myAct + 1] += scorebase;
                    break;
                }
            }

        // �ָ���Ϸ״̬����������Ǳ��غϣ�
        while (count-- > 0)
            gameField.PopState();
    }

    int FindShortestPath(const Pacman::GameField & gamefield, const Pacman::FieldProp & Begin, const Pacman::FieldProp & End)
    {
        int h = gamefield.height; int w = gamefield.width;//��ȡheight width
        if (Begin.row >= h || Begin.row < 0 || Begin.col >= w || Begin.col < 0)
            throw runtime_error("�������겻�Ϸ�");
        if (End.row >= h || End.row < 0 || End.col >= w || End.col < 0)
            throw runtime_error("�������겻�Ϸ�");
        if (Begin == End) return 0;
        bool decide[FIELD_MAX_HEIGHT][FIELD_MAX_WIDTH];
        memset(decide, 0, sizeof(decide));
        queue<Pacman::FieldProp > a;
        a.push(Pacman::FieldProp(Begin.row, Begin.col));
        decide[Begin.row][Begin.col] = true;
        int num = 1, path = 0, nextnum = 0;
        while (path <= h*w)
        {
            if (a.empty()) break;
            ++path;
            for (int i = 0; i < num; ++i)
            {
                Pacman::FieldProp& temp = a.front();
                if (!(gamefield.fieldStatic[temp.row][temp.col] & Pacman::wallNorth) && !decide[(temp.row + h - 1) % h][temp.col])
                {
                    if ((temp.row + h - 1) % h == End.row&&temp.col == End.col) return path;
                    ++nextnum; a.push(Pacman::FieldProp((temp.row + h - 1) % h, temp.col));
                    decide[(temp.row + h - 1) % h % h][temp.col] = true;
                }
                if (!(gamefield.fieldStatic[temp.row][temp.col] & Pacman::wallSouth) && !decide[(temp.row + 1) % h][temp.col])
                {
                    if ((temp.row + 1) % h == End.row&&temp.col == End.col) return path;
                    ++nextnum; a.push(Pacman::FieldProp((temp.row + 1) % h, temp.col));
                    decide[(temp.row + 1) % h][temp.col] = true;
                }
                if (!(gamefield.fieldStatic[temp.row][temp.col] & Pacman::wallEast) && !decide[temp.row][(temp.col + 1) % w])
                {
                    if (temp.row == End.row && (temp.col + 1) % w == End.col) return path;
                    ++nextnum; a.push(Pacman::FieldProp(temp.row, (temp.col + 1) % w));
                    decide[temp.row][(temp.col + 1) % w] = true;
                }
                if (!(gamefield.fieldStatic[temp.row][temp.col] & Pacman::wallWest) && !decide[temp.row][(temp.col + w - 1) % w])
                {
                    if (temp.row == End.row && (temp.col + w - 1) % w % w == End.col) return path;
                    ++nextnum; a.push(Pacman::FieldProp(temp.row, (temp.col + w - 1) % w));
                    decide[temp.row][(temp.col + w - 1) % w] = true;
                }
                a.pop();
            }
            num = nextnum; nextnum = 0;
        }
        return 999999;
    }

    int FindSmallestIndex(int *a, int n)
    {
        int p = a[0], ans = 0;
        for (int i = 1; i < n; i++)
            if (a[i] < p)
            {
                p = a[i];
                ans = i;
            }
        return ans;
    }

    int FindNearGeneratorIndex(Pacman::GameField &gameField, int myID)
    {
        int tmpDistance[MAX_GENERATOR_COUNT] = {0};
        for (int i = 0; i < MAX_GENERATOR_COUNT; i++)
            tmpDistance[i] = FindShortestPath(gameField, gameField.players[myID], gameField.generators[i]);
        return FindSmallestIndex(tmpDistance, MAX_GENERATOR_COUNT);
    }

    void FruitFirstPlay(Pacman::GameField &gameField, int myID, int step)
    {
        int myAct = -1;
        if (step == DEPTH - 1 || !gameField.NextTurn())
        {
            if (TempValue > TempMaxValue)
            {
                TempMaxValue = TempValue;
                MyDFSAct = TempAction[0];
            }
            return;
        }
        for (Pacman::Direction d = Pacman::up; d < 4; ++d)
        {
            if (gameField.ActionValid(myID, d))
            {
                TempAction[step] = d;
                BeginPosition = gameField.players[myID];
                Pacman::GridContentType &content = gameField.fieldContent[BeginPosition.col][BeginPosition.row];
                if (content & Pacman::largeFruit)
                {
                    TempValue += 5;
                    content &= ~Pacman::largeFruit;
                }
                if (content & Pacman::smallFruit)
                {
                    TempValue += 2;
                    content &= ~Pacman::smallFruit;
                }
                //�������Ԥ��
                for (int i = 0; i < MAX_PLAYER_COUNT; i++)
                {
                    if (gameField.players[i].dead || i == myID)
                        continue;
                    else if (FindShortestPath(gameField, gameField.players[i], gameField.players[myID]) < ShortestDangerDistance)
                        TempValue -= 8;
                }
                FruitFirstPlay(gameField, myID, step + 1);
                gameField.PopState();
            }
        }
    }
}


namespace QhHelper
{
    string data, globalData; // ���ǻغ�֮����Դ��ݵ���Ϣ
    stringstream ss;
    Pacman::Direction avoid = Pacman::stay;
    string tmpGlobalData;
    bool OffRoadPosition[FIELD_MAX_HEIGHT][FIELD_MAX_WIDTH] = {0};

    int forwardsteps[6][2];
    const double stopnumber = 999999;//��ֵ�����ʹ·�����
    int h;//gamefield.height
    int w;//gamefiled.width
    int myrow;//gamefield.players[myID].row
    int mycol;//gamefiled.players[myID].col
    int mystrength;//gamefiled.players[myID].strength
    int mypowerUpLeft;//gamefield.players[myID].powerUpLeft
    void Qhset(const Pacman::GameField& gamefield, int myID)
    {
        h = gamefield.height; w = gamefield.width;
        myrow = gamefield.players[myID].row;
        mycol = gamefield.players[myID].col;
        mystrength = gamefield.players[myID].strength;
        mypowerUpLeft = gamefield.players[myID].powerUpLeft;
    }
    //code �� value ����һ�ɲ���double
    //r,cΪbfs��ʼ���꣬value�ǹ�ֵ���������ã����ڴ��ݸ�op��op���Ƕ�Ӧ�ľ���Ĳ�������
    //op���ص���boolֵ ����false����bfs����������true����bfs����
    //r,cΪbfs��ʼ���꣬value�ǹ�ֵ���������ã����ڴ��ݸ�op��op���Ƕ�Ӧ�ľ���Ĳ�������
    //op���ص���boolֵ ����false����bfs����������true����bfs����
    bool IfEmpty;     //�����ж�������Χ֮���Ƿ����fruit ����value=0�����е�Ϊ��
                      //false ���� ���� true���� ��

    void FieldSetFlag(const Pacman::GameField &gamefield)
    {
        for (int i = 0; i < h;i ++)
            for (int j = 0; j < w; j++)
            {
                if(OffRoadPosition[i][j])
                    continue;
                int cnt[4] = {0};
                Pacman::Direction dir;
                if (gamefield.fieldContent[i][j] & Pacman::wallEast)
                    cnt[1]++;
                if (gamefield.fieldContent[i][j] & Pacman::wallWest)
                    cnt[3]++;
                if (gamefield.fieldContent[i][j] & Pacman::wallNorth)
                    cnt[0]++;
                if (gamefield.fieldContent[i][j] & Pacman::wallSouth)
                    cnt[2]++;
                if (cnt[0] + cnt[1] + cnt[2] + cnt[3] == 3)
                    OffRoadPosition[i][j] = true;
            }
    }
    template<class func>
    void BfsSearch(const Pacman::GameField &gamefield, int r, int c, double &value, func op, int myID)
    {
        bool decide[FIELD_MAX_HEIGHT][FIELD_MAX_WIDTH];
        memset(decide, 0, sizeof(decide));
        queue<Pacman::FieldProp > a;
        int num = 1, nextnum = 0; int path = 0;
        a.push(Pacman::FieldProp(r, c));
        decide[r][c] = true;
        if (!op(gamefield, r, c, value, path, myID))
            return;
        while (true)
        {
            ++path;
            if (a.empty()) break;
            for (int i = 0; i < num; ++i)
            {
                Pacman::FieldProp& temp = a.front();
                if (!(gamefield.fieldStatic[temp.row][temp.col] & Pacman::wallNorth) && !decide[(temp.row + h - 1) % h][temp.col])
                {
                    if (!op(gamefield, (temp.row + h - 1) % h, temp.col, value, path, myID)) return;
                    ++nextnum; a.push(Pacman::FieldProp((temp.row + h - 1) % h, temp.col));
                    decide[(temp.row + h - 1) % h % h][temp.col] = true;
                }
                if (!(gamefield.fieldStatic[temp.row][temp.col] & Pacman::wallSouth) && !decide[(temp.row + 1) % h][temp.col])
                {
                    if (!op(gamefield, (temp.row + 1) % h, temp.col, value, path, myID)) return;
                    ++nextnum; a.push(Pacman::FieldProp((temp.row + 1) % h, temp.col));
                    decide[(temp.row + 1) % h][temp.col] = true;
                }
                if (!(gamefield.fieldStatic[temp.row][temp.col] & Pacman::wallEast) && !decide[temp.row][(temp.col + 1) % w])
                {
                    if (!op(gamefield, temp.row, (temp.col + 1) % w, value, path, myID)) return;
                    ++nextnum; a.push(Pacman::FieldProp(temp.row, (temp.col + 1) % w));
                    decide[temp.row][(temp.col + 1) % w] = true;
                }
                if (!(gamefield.fieldStatic[temp.row][temp.col] & Pacman::wallWest) && !decide[temp.row][(temp.col + w - 1) % w])
                {
                    if (!op(gamefield, temp.row, (temp.col + w - 1) % w, value, path, myID)) return;
                    ++nextnum; a.push(Pacman::FieldProp(temp.row, (temp.col + w - 1) % w));
                    decide[temp.row][(temp.col + w - 1) % w] = true;
                }
                a.pop();
            }
            num = nextnum; nextnum = 0;
        }
        return;
    }

    bool operator ==(const Pacman::FieldProp & a, const Pacman::FieldProp & b)
    {
        return a.col == b.col&&a.row == b.row ? true : false;
    }
    //return 99999 �������㲻������
    //�����������·����
    int Terr, Terc;//������ʱ����Begin �� row �� col��ȫ��var
                   //���·��bfsop
    bool FindShortestPathBfsop(const Pacman::GameField & gamefield, int r, int c, double & value, int path, int myID)
    {
        if (r == Terr && c == Terc)
        {
            value = path; return false;
        }
        else return true;
    }
    int FindShortestPathBfs(const Pacman::FieldProp & Begin, const Pacman::FieldProp & End, const Pacman::GameField & gamefield)
    {
        Terr = End.row; Terc = End.col;//��ʼ��
        double value = stopnumber;//��ʼ��ֵΪ��ͨ
        BfsSearch(gamefield, Begin.row, Begin.col, value, FindShortestPathBfsop, -1);//myID��ֵ-1������																			 
        return value;
    }


    //�ж��Ƿ������·����fruit
    void EnemyOffroad(const Pacman::GameField& gamefield, int myID, bool decide[FIELD_MAX_HEIGHT][FIELD_MAX_WIDTH])
    {
        for (int i = 0; i<4; ++i)
            if (i != myID)
                if (!gamefield.players[i].dead&&gamefield.players[i].strength > mystrength)
                {
                    int tempr = gamefield.players[i].row, tempc = gamefield.players[i].col;
                    decide[tempr][tempc] = true;
                    if (!(gamefield.fieldStatic[tempr][tempc] & Pacman::wallNorth))
                        decide[(tempr + h - 1) % h][tempc] = true;
                    if (!(gamefield.fieldStatic[tempr][tempc] & Pacman::wallSouth))
                        decide[(tempr + 1) % h][tempc] = true;
                    if (!(gamefield.fieldStatic[tempr][tempc] & Pacman::wallEast))
                        decide[tempr][(tempc + 1) % w] = true;
                    if (!(gamefield.fieldStatic[tempr][tempc] & Pacman::wallWest))
                        decide[tempr][(tempc - 1 + w) % w] = true;
                }
    }
    //
    struct Eaten
    {
        int zero, one, two, three;
        int fsmall, flarge;
        int path;
        Eaten()
        {
            zero = one = two = three = 0;
            fsmall = flarge = 0;
            path = 0;
        }
        void setfalse()
        {
            zero = one = two = three = 0;
            fsmall = flarge = 0;
            path = 0;
        }
        operator bool()
        {
            if (zero || one || two || three) return true;
            else return false;
        }
        int sum()
        {
            return zero + one + two + three;
        }
        int choose()
        {
            if (zero) return 0;
            if (one) return 1;
            if (two) return 2;
            if (three) return 3;
            return -1;
        }
    };
    Eaten eatenenemy;//��¼׼���Եĵ��˵���Զ����·��
                     /*
                     fruitthink 1 ����fruit����fruit����true 0 ͬʱ��¼���˵�һЩ��Ϣ
                     */
    bool OffRoad(const Pacman::GameField& gamefield, Pacman::Direction d, int myID, int fruitthink, int Depth = 6)//xǰ������,Depth Ϊ�������
    {
        const int searchpath = 6;
        int r = (myrow + h + Pacman::dy[d]) % h, c = (mycol + w + Pacman::dx[d]) % w;
        bool decide[FIELD_MAX_HEIGHT][FIELD_MAX_WIDTH];
        memset(decide, false, sizeof(decide));
        queue<Pacman::FieldProp > a;
        a.push(Pacman::FieldProp(r, c));
        decide[myrow][mycol] = true;
        decide[r][c] = true;
        int num = 1, nextnum = 0;
        if (!fruitthink)//�жϻ᲻�ᱻ�ƽ���·ʱ�����Լ�����ǿ�ĵ�����Χ���·��
            EnemyOffroad(gamefield, myID, decide);
        while (Depth > 0)
        {
            --Depth;
            for (int i = 0; i < num; ++i)
            {
                Pacman::FieldProp& temp = a.front();
                if (fruitthink == 1 && gamefield.fieldContent[temp.row][temp.col] & (Pacman::smallFruit | Pacman::largeFruit))
                    return false;
                if (fruitthink == 0)
                {
                    if (gamefield.fieldContent[temp.row][temp.col] & Pacman::smallFruit)
                        eatenenemy.fsmall += 1;
                    if (gamefield.fieldContent[temp.row][temp.col] & Pacman::largeFruit)
                        eatenenemy.flarge += 1;
                    if (myID != 0 && gamefield.fieldContent[temp.row][temp.col] & Pacman::player1)
                        if (gamefield.players[0].strength < mystrength)
                            eatenenemy.zero = 1;
                    if (myID != 1 && gamefield.fieldContent[temp.row][temp.col] & Pacman::player2)
                        if (gamefield.players[1].strength < mystrength)
                            eatenenemy.one = 1;
                    if (myID != 2 && gamefield.fieldContent[temp.row][temp.col] & Pacman::player3)
                        if (gamefield.players[2].strength < mystrength)
                            eatenenemy.two = 1;
                    if (myID != 3 && gamefield.fieldContent[temp.row][temp.col] & Pacman::player4)
                        if (gamefield.players[3].strength < mystrength)
                            eatenenemy.three = 1;
                }
                if (!(gamefield.fieldStatic[temp.row][temp.col] & Pacman::wallNorth) && !decide[(temp.row + h - 1) % h][temp.col])
                {
                    ++nextnum; a.push(Pacman::FieldProp((temp.row + h - 1) % h, temp.col));
                    decide[(temp.row + h - 1) % h][temp.col] = true;
                }
                if (!(gamefield.fieldStatic[temp.row][temp.col] & Pacman::wallSouth) && !decide[(temp.row + 1) % h][temp.col])
                {
                    ++nextnum; a.push(Pacman::FieldProp((temp.row + 1) % h, temp.col));
                    decide[(temp.row + 1) % h][temp.col] = true;
                }
                if (!(gamefield.fieldStatic[temp.row][temp.col] & Pacman::wallEast) && !decide[temp.row][(temp.col + 1) % w])
                {
                    ++nextnum; a.push(Pacman::FieldProp(temp.row, (temp.col + 1) % w));
                    decide[temp.row][(temp.col + 1) % w] = true;
                }
                if (!(gamefield.fieldStatic[temp.row][temp.col] & Pacman::wallWest) && !decide[temp.row][(temp.col + w - 1) % w])
                {
                    ++nextnum; a.push(Pacman::FieldProp(temp.row, (temp.col + w - 1) % w));
                    decide[temp.row][(temp.col + w - 1) % w] = true;
                }
                a.pop();
            }
            num = nextnum; nextnum = 0;
            if (num == 0)
            {
                eatenenemy.path = searchpath - Depth;
                return true;
            }
        }
        eatenenemy.setfalse();
        return false;
    }
    //�ж��Լ��Ƿ��п��ܱ�����׷����·�Ե�
    //�ǵĻ�����Ҳ���ܳ������������
    //����Ƿ���true else false
    //�߽���·���˵ĺ���
    int OffRoadEat(const Pacman::GameField& gamefield)
    {
        if (eatenenemy.sum() > 2 || eatenenemy.sum() == 0)
            return -1;
        int eaten = eatenenemy.choose();
        if (eaten >= 0)
        {
            int tempUP = 0;
            tempUP += eatenenemy.fsmall;
            if (eatenenemy.flarge)
                tempUP += 10;
            if (gamefield.players[eaten].strength + tempUP >= mystrength)
                return -1;
            else
                return eaten;
        }
        return -1;
    }
    //����һ���������Ƿ���Σ�յ��˴���
    struct GetFruit:Pacman::FieldProp
    {
        Pacman::GridContentType fruit;//����
        int path;//·��
        GetFruit(Pacman::GridContentType a, int b, int r, int c):fruit(a), path(b), Pacman::FieldProp(r, c) {}
    };
    //ץȡ�����һ��fruit smallfruit��largefruit�ֿ�����
    GetFruit GetFruitPath(Pacman::Direction d, const Pacman::GameField& gamefield, int myID, Pacman::GridContentType fruit)
    {
        int MaxDepth = 6;
        bool decide[FIELD_MAX_HEIGHT][FIELD_MAX_WIDTH];
        memset(decide, false, sizeof(decide));
        int r = (myrow + h + Pacman::dy[d]) % h;
        int c = (mycol + w + Pacman::dx[d]) % w;
        if (gamefield.fieldContent[r][c] & fruit)
            return GetFruit(fruit, 1, r, c);
        decide[myrow][mycol] = true;
        queue <Pacman::FieldProp> a;
        a.push(Pacman::FieldProp(r, c));
        decide[r][c] = true;
        int num = 1, nextnum = 0, path = 1;
        while (true)
        {
            ++path;
            if (path > MaxDepth) return GetFruit(Pacman::empty, 0, -1, -1);
            for (int i = 0; i < num; ++i)
            {
                Pacman::FieldProp & temp = a.front();
                if (!(gamefield.fieldStatic[temp.row][temp.col] & Pacman::wallNorth) && !decide[(temp.row + h - 1) % h][temp.col])
                {
                    if (gamefield.fieldContent[(temp.row + h - 1) % h][temp.col] & fruit)
                        return GetFruit(fruit, path, (temp.row + h - 1) % h, temp.col);
                    ++nextnum; a.push(Pacman::FieldProp((temp.row + h - 1) % h, temp.col));
                }
                if (!(gamefield.fieldStatic[temp.row][temp.col] & Pacman::wallSouth) && !decide[(temp.row + 1) % h][temp.col])
                {
                    if (gamefield.fieldContent[(temp.row + 1) % h][temp.col] & fruit)
                        return GetFruit(fruit, path, (temp.row + 1) % h, temp.col);
                    ++nextnum; a.push(Pacman::FieldProp((temp.row + 1) % h, temp.col));
                }
                if (!(gamefield.fieldStatic[temp.row][temp.col] & Pacman::wallEast) && !decide[temp.row][(temp.col + 1) % w])
                {
                    if (gamefield.fieldContent[temp.row][(temp.col + 1) % w] & fruit)
                        return GetFruit(fruit, path, temp.row, (temp.col + 1) % w);
                    ++nextnum; a.push(Pacman::FieldProp(temp.row, (temp.col + 1) % w));
                }
                if (!(gamefield.fieldStatic[temp.row][temp.col] & Pacman::wallWest) && !decide[temp.row][(temp.col + w - 1) % w])
                {
                    if (gamefield.fieldContent[temp.row][(temp.col + w - 1) % w] & fruit)
                        return GetFruit(fruit, path, temp.row, (temp.col + w - 1) % w);
                    ++nextnum; a.push(Pacman::FieldProp(temp.row, (temp.col + w - 1) % w));
                }
                a.pop();
            }
            num = nextnum; nextnum = 0;
            if (num == 0) return GetFruit(Pacman::empty, 0, -1, -1);
        }
    }
    //�жϵ��˵���в��
    //false û��в true  ����в
    bool enemythreat(const Pacman::GameField& gamefield, int k, int myID, int r, int c, GetFruit & target)//k��enemy��Ӧ���
    {
        if (k == myID) return false;
        if (target.fruit == Pacman::empty) return false;
        int enemybyte = pow(2, k);
        int benstr = 0;
        if (target.fruit == Pacman::smallFruit) benstr = 1;
        else benstr = 10;
        if (gamefield.fieldContent[r][c] & enemybyte)
        {
            if (FindShortestPathBfs(Pacman::FieldProp(r, c), Pacman::FieldProp(target.row, target.col), gamefield)
                < FindShortestPathBfs(Pacman::FieldProp(myrow, mycol), Pacman::FieldProp(target.row, target.col), gamefield))
            {
                if (gamefield.players[k].strength + benstr  > mystrength) return true;
                else return false;
            }
            else
            {
                if (mystrength + benstr <  gamefield.players[k].strength) return true;
                else return false;
            }
        }
        return false;
    }
    //�����������ĵ��˱�� 
    //-1����û�ҵ�
    int SearchEnemy(const Pacman::GameField& gamefield, int myID, GetFruit & target)
    {
        bool decide[FIELD_MAX_HEIGHT][FIELD_MAX_WIDTH];
        memset(decide, false, sizeof(decide));
        decide[myrow][mycol] = true;
        queue <Pacman::FieldProp> a;
        a.push(Pacman::FieldProp(myrow, mycol));
        int num = 1, nextnum = 0, path = 0;
        while (true)
        {
            ++path;
            if (path > 2 * target.path) return -1;
            for (int i = 0; i < num; ++i)
            {
                Pacman::FieldProp & temp = a.front();
                if (!(gamefield.fieldStatic[temp.row][temp.col] & Pacman::wallNorth) && !decide[(temp.row + h - 1) % h][temp.col])
                {
                    for (int j = 0; j < 4; ++j)
                        if (enemythreat(gamefield, j, myID, (temp.row - 1 + h) % h, temp.col, target))
                            return j;
                    ++nextnum; a.push(Pacman::FieldProp((temp.row - 1 + h) % h, temp.col));
                    decide[(temp.row + h - 1) % h][temp.col] = true;
                }
                if (!(gamefield.fieldStatic[temp.row][temp.col] & Pacman::wallSouth) && !decide[(temp.row + 1) % h][temp.col])
                {
                    for (int j = 0; j < 4; ++j)
                        if (enemythreat(gamefield, j, myID, (temp.row + 1) % h, temp.col, target))
                            return j;
                    ++nextnum; a.push(Pacman::FieldProp((temp.row + 1) % h, temp.col));
                    decide[(temp.row + 1) % h][temp.col] = true;
                }
                if (!(gamefield.fieldStatic[temp.row][temp.col] & Pacman::wallEast) && !decide[temp.row][(temp.col + 1) % w])
                {
                    for (int j = 0; j < 4; ++j)
                        if (enemythreat(gamefield, j, myID, temp.row, (temp.col + 1) % w, target))
                            return j;
                    ++nextnum; a.push(Pacman::FieldProp(temp.row, (temp.col + 1) % w));
                    decide[temp.row][(temp.col + 1) % w] = true;
                }
                if (!(gamefield.fieldStatic[temp.row][temp.col] & Pacman::wallWest) && !decide[temp.row][(temp.col + w - 1) % w])
                {
                    for (int j = 0; j < 4; ++j)
                        if (enemythreat(gamefield, j, myID, temp.row, (temp.col + w - 1) % w, target))
                            return j;
                    ++nextnum; a.push(Pacman::FieldProp(temp.row, (temp.col + w - 1) % w));
                    decide[temp.row][(temp.col + w - 1) % w] = true;
                }
                a.pop();
            }
            num = nextnum; nextnum = 0;
            if (num == 0) return -1;
        }
    }

    //����true ����в ����false ����в
    bool LifeSaveEnemy(Pacman::Direction d, const Pacman::GameField& gamefield, int myID)
    {
        GetFruit sfruit = GetFruitPath(d, gamefield, myID, Pacman::smallFruit),
            lfruit = GetFruitPath(d, gamefield, myID, Pacman::largeFruit);
        int person1 = SearchEnemy(gamefield, myID, sfruit),
            person2 = SearchEnemy(gamefield, myID, lfruit);
        if (person1 == -1 && person2 == -1) return false;
        else return true;
    }

    //1.�ж��Ƿ�������õ�����ͬ
    //2.�ж��Ƿ��������ͬ��ᱻܳ
    //3.����true��������� ������false��������
    bool LifeSave(Pacman::Direction d, const Pacman::GameField& gamefield, int myID)
    {
        int r = (myrow + h + Pacman::dy[d]) % h;
        int c = (mycol + w + Pacman::dx[d]) % w;
        bool del1 = OffRoad(gamefield, d, myID, 1);//false �ж��ӻ���ͨ true �޶����Ҳ�ͨ
        bool del2 = OffRoad(gamefield, d, myID, 0);//true ��ͨ false ͨ ˳����eatenenemy���и�ֵ
        bool del3 = LifeSaveEnemy(d, gamefield, myID);//true����в false����в
        if (del2)
        {
            if (del3) return false;
            if (!del1) return true;
            if (eatenenemy) return true;
            return false;
        }
        else return true;
    }

    //��ȡ��ֵ
    void FruitofEnemy(const Pacman::GameField & gamefield, const double & k1, const double &k2, const double &k3, int * dis, double & value, int myID, int enemyID)
    {
        const int Maxstep = 20;
        if (myID == enemyID) return;
        if (!gamefield.players[enemyID].dead)
        {
            if (dis[myID]<dis[enemyID] && gamefield.players[enemyID].strength>mystrength)
                value += k3 / (dis[enemyID] + 1);
            if (dis[myID] >= dis[enemyID])
            {
                if (gamefield.players[enemyID].strength > mystrength)
                    value += k2 / (dis[enemyID] + 1);
                else value += k1 / (dis[enemyID] + 1);
            }
        }
    }
    double ValueTake2(const Pacman::GameField & gamefield, int r, int c, int myID, int path)
    {
        double value = 0;
        const double neargenerator = 1;
        const int maxpath = 20;
        const double disaffect = 0.05;
        if (gamefield.fieldStatic[(r + h - 1) % h][c] & Pacman::generator)
            value += neargenerator + disaffect*(maxpath - path);
        if (gamefield.fieldStatic[(r + 1) % h][c] & Pacman::generator)
            value += neargenerator + disaffect*(maxpath - path);
        if (gamefield.fieldStatic[r][(c + w - 1) % w] & Pacman::generator)
            value += neargenerator + disaffect*(maxpath - path);
        if (gamefield.fieldStatic[r][(c + 1) % w] & Pacman::generator)
            value += neargenerator + disaffect*(maxpath - path);
        return value;
    }
    double ValueTake(const Pacman::GameField & gamefield, int r, int c, int myID, int path)
    {
        const int Maxstep = 20;
        const Pacman::Player & zero = gamefield.players[0], one = gamefield.players[1],
            two = gamefield.players[2], three = gamefield.players[3];
        const double smallvalue = 10;
        double largevalue = 0;//why ����const ?��Ϊ����Ҫ��ֵ
        if (mypowerUpLeft > 3)
            largevalue = 10;
        //largevalue = 0.12*(gamefield.LARGE_FRUIT_DURATION - mypowerUpLeft);
        else
            largevalue = 100;
        //const int generatvalue = 5;
        const double threat = -40;
        //const double position = -5;
        const double sFruitofsEnemy = 0;//���˾����ʱ������λ�öԹ��ӹ�ֵӰ�����
        const double sFruitoflEnemy = 0;//���˾����ʱǿ����λ�öԹ��ӹ�ֵӰ�����
        const double lFruitoflEnemy = 0;//���˾���Զʱǿ����λ�öԹ��ӹ�ֵ����Ӱ��
        double value = 0; int myByte = pow(2, myID);
        int dis[4];
        /*dis[0] = FindShortestPathBfs(Pacman::FieldProp(r, c), Pacman::FieldProp(zero.row, zero.col), gamefield);
        dis[1] = FindShortestPathBfs(Pacman::FieldProp(r, c), Pacman::FieldProp(one.row, one.col), gamefield);
        dis[2] = FindShortestPathBfs(Pacman::FieldProp(r, c), Pacman::FieldProp(two.row, two.col), gamefield);
        dis[3] = FindShortestPathBfs(Pacman::FieldProp(r, c), Pacman::FieldProp(three.row, three.col), gamefield);*/
        if (gamefield.fieldContent[r][c] & Pacman::smallFruit)
        {
            value += smallvalue;
            /*for (int i = 0; i < 4; ++i)
            FruitofEnemy(gamefield, sFruitofsEnemy, sFruitoflEnemy, lFruitoflEnemy, dis, value, myID, i);*/
        }
        if (gamefield.fieldContent[r][c] & Pacman::largeFruit)
        {
            value += largevalue;
            //for (int i = 0; i < 4; ++i)
            //FruitofEnemy(gamefield, sFruitofsEnemy, sFruitoflEnemy, lFruitoflEnemy, dis, value, myID, i);
        }
        //if (gamefield.fieldContent[r][c] & Pacman::generator) value += generatvalue;
        // ��� ��������ǿ�ĵ��� ��������֮�� ˵ɶ ���ܣ�
        //player.dead false��Ҵ��
        //const double sEnemy = 0.5;
        //if (path > 0) return value;
        if ((gamefield.fieldContent[r][c] & Pacman::player1) && Pacman::player1 != myByte && !zero.dead)
            if (zero.strength > mystrength)
            {
                int dis = FindShortestPathBfs(Pacman::FieldProp(myrow, mycol), Pacman::FieldProp(zero.row, zero.col), gamefield);
                if (dis < 3)
                    value -= stopnumber;
                else
                    value += threat;
            }

        if ((gamefield.fieldContent[r][c] & Pacman::player2) && Pacman::player2 != myByte && !one.dead)
            if (one.strength > mystrength)
            {
                int dis = FindShortestPathBfs(Pacman::FieldProp(myrow, mycol), Pacman::FieldProp(one.row, one.col), gamefield);
                if (dis < 3)
                    value -= stopnumber;
                else
                    value += threat;
            }

        if ((gamefield.fieldContent[r][c] & Pacman::player3) && Pacman::player3 != myByte && !two.dead)
            if (two.strength > mystrength)
            {
                int dis = FindShortestPathBfs(Pacman::FieldProp(myrow, mycol), Pacman::FieldProp(two.row, two.col), gamefield);
                if (dis < 3)
                    value -= stopnumber;
                else
                    value += threat;
            }

        if ((gamefield.fieldContent[r][c] & Pacman::player4) && Pacman::player4 != myByte && !three.dead)
            if (three.strength > mystrength)
            {
                int dis = FindShortestPathBfs(Pacman::FieldProp(myrow, mycol), Pacman::FieldProp(three.row, three.col), gamefield);
                if (dis < 3)
                    value -= stopnumber;
                else
                    value += threat;
            }

        return value;
    }
    //BfsSearch_��op����
    inline bool BfsValueop(const Pacman::GameField &gamefield, int r, int c, double &value, int path, int myID)
    {
        const int Maxstep = 20;
        if (path > Maxstep) return false;
        //if (r == myrow && c == mycol) return true;
        double pointvalue = ValueTake(gamefield, r, c, myID, path);//���ظõ�Գ�����Ĺ�ֵ
        double pointvalue2 = ValueTake2(gamefield, r, c, myID, path);//��generator����λ�õĹ�ֵ
        value += pointvalue*(Maxstep - path);
        value += pointvalue2;
        if (pointvalue != 0 || pointvalue2 != 0) IfEmpty = false;
        return true;
    }
    double BfsValue(Pacman::FieldProp Begin, const Pacman::GameField & gamefield, int myID)
    {
        IfEmpty = true;//reset variable  IfEmpty
        double value = 0;//�������صļ�ֵ,���ڹ�ֵ���Ծ��� 
        BfsSearch(gamefield, Begin.row, Begin.col, value, BfsValueop, myID);
        return value;
    }
    //BFS̽�����޲�
    //�ֲ�̰�� ������Ĺ���

    inline bool FirstTakenop(const Pacman::GameField & gamefield, int r, int c, double& value, int path, int myID)
    {
        int Maxstep = 20;
        if (path > Maxstep)
        {
            value = 0; return false;
        }
        if (gamefield.fieldContent[r][c] & (Pacman::smallFruit | Pacman::largeFruit))
        {
            value = Maxstep - path; return false;
        }
        else return true;
    }
    //������õ���������Ĺ���
    int FirstTakenBfs(Pacman::FieldProp Begin, const Pacman::GameField & gamefield, int myID)
    {
        double value = 0;
        BfsSearch(gamefield, Begin.row, Begin.col, value, FirstTakenop, myID);
        return value;
    }

    //�ж���в�Ƿ������ǰ
    //�ǵĻ�����Ҳ���ܳ������������
    //����Ƿ���true else false
    //�����ܵ��������
    Pacman::Direction Direct(int turnID)
    {
        int frow = forwardsteps[(turnID - 1) % 6][0], fcol = forwardsteps[(turnID - 1) % 6][1];
        if (frow == (myrow + h - 1) % h) return Pacman::up;
        if (frow == (myrow + 1) % h) return Pacman::down;
        if (fcol == (mycol + 1) % w) return Pacman::right;
        if (fcol == (mycol + w - 1) % w) return Pacman::left;
    }
    void Globalread(int turnID)
    {
        memset(forwardsteps, 0, sizeof(forwardsteps));
        if (turnID > 1)
        {
            char temp;
            ss << globalData;
            for (int i = 0; i < 6; ++i)
                ss >> forwardsteps[i][0] >> temp >> forwardsteps[i][1] >> temp;
        }
        forwardsteps[(turnID - 6) % 6][0] = myrow;
        forwardsteps[(turnID - 6) % 6][1] = mycol;
        for (int i = 0; i<6; ++i)
            ss << forwardsteps[i][0] << ',' << forwardsteps[i][1] << ',';
        ss >> globalData;
        if (turnID < 6) return;
        int i;
        for (i = 0; i < 4; ++i)
            if (forwardsteps[i][0] != forwardsteps[i + 2][0] || forwardsteps[i][1] != forwardsteps[i + 2][1])
                break;
        if (i == 4)
            avoid = Direct(turnID);
    }
    void Y_Globalread(int turnID)
    {
        if (turnID == 1)
        {
            memset(forwardsteps, 0, sizeof(forwardsteps));
            globalData = "\0";
            tmpGlobalData = "\0";
        }
        else
        {
            char tmp;
            tmpGlobalData = globalData;
            while (tmpGlobalData.find(',') != string::npos)
            {
                int pos = tmpGlobalData.find(',');
                tmpGlobalData[pos] = ' ';
            }
            ss << tmpGlobalData;
            for (int i = 0; i < 6; i++)
                ss >> forwardsteps[i][0] >> forwardsteps[i][1];
        }
        forwardsteps[turnID][0] = myrow;
        forwardsteps[turnID][1] = mycol;
        for (int i = 0; i < 6; i++)
            ss << forwardsteps[i][0] << ',' << forwardsteps[i][1] << ',';
        ss >> tmpGlobalData;
        globalData = tmpGlobalData;
    }
    Pacman::Direction AIOutput(const Pacman::GameField &gamefield, int myID)
    {
        Qhset(gamefield, myID);
        //Globalread(gamefield.turnID);
        const double mineat = 8;
        const double bfs = 1;
        const double firstfruit = 500;
        double eatvalue = 0;
        double value = -100 * stopnumber;
        Pacman::Direction target = Pacman::stay;
        if (!(gamefield.fieldStatic[myrow][mycol] & Pacman::wallNorth))
            if (LifeSave(Pacman::up, gamefield, myID))//�жϲ�����·��û������Σ��
            {
                int eat = OffRoadEat(gamefield);
                if (gamefield.players[eat].strength > eatvalue && gamefield.players[eat].strength > mineat)
                {
                    eatvalue = gamefield.players[eat].strength;
                    target = Pacman::up;
                }
                if (eatvalue == 0)
                {
                    double k1 = BfsValue(Pacman::FieldProp((myrow + h - 1) % h, mycol), gamefield, myID)*bfs;
                    double k2 = FirstTakenBfs(Pacman::FieldProp((myrow + h - 1) % h, mycol), gamefield, myID)*firstfruit;
                    double tempvalue = k1 + k2;
                    if (value < tempvalue && !IfEmpty&& avoid != Pacman::up)
                    {
                        value = tempvalue; target = Pacman::up;
                    }
                }
            }
        eatenenemy.setfalse();
        if (!(gamefield.fieldStatic[myrow][mycol] & Pacman::wallEast))
            if (LifeSave(Pacman::right, gamefield, myID))
            {
                int eat = OffRoadEat(gamefield);
                if (gamefield.players[eat].strength > eatvalue && gamefield.players[eat].strength > mineat)
                {
                    eatvalue = gamefield.players[eat].strength;
                    target = Pacman::right;
                }
                if (eatvalue == 0)
                {
                    double k1 = BfsValue(Pacman::FieldProp(myrow, (mycol + 1) % w), gamefield, myID)*bfs;
                    double k2 = FirstTakenBfs(Pacman::FieldProp(myrow, (mycol + 1) % w), gamefield, myID)*firstfruit;
                    double tempvalue = k1 + k2;
                    if (value < tempvalue && !IfEmpty && avoid != Pacman::right)
                    {
                        value = tempvalue; target = Pacman::right;
                    }
                }
            }
        eatenenemy.setfalse();
        if (!(gamefield.fieldStatic[myrow][mycol] & Pacman::wallSouth))
            if (LifeSave(Pacman::down, gamefield, myID))
            {
                int eat = OffRoadEat(gamefield);
                if (gamefield.players[eat].strength > eatvalue && gamefield.players[eat].strength > mineat)
                {
                    eatvalue = gamefield.players[eat].strength;
                    target = Pacman::down;
                }
                if (eatvalue == 0)
                {
                    double k1 = BfsValue(Pacman::FieldProp((myrow + 1) % h, mycol), gamefield, myID)*bfs;
                    double k2 = FirstTakenBfs(Pacman::FieldProp((myrow + 1) % h, mycol), gamefield, myID)*firstfruit;
                    double tempvalue = k1 + k2;
                    if (value < tempvalue && !IfEmpty && avoid != Pacman::down)
                    {
                        value = tempvalue; target = Pacman::down;
                    }
                }
            }
        eatenenemy.setfalse();
        if (!(gamefield.fieldStatic[myrow][mycol] & Pacman::wallWest))
            if (LifeSave(Pacman::left, gamefield, myID))
            {
                int eat = OffRoadEat(gamefield);
                if (gamefield.players[eat].strength > eatvalue && gamefield.players[eat].strength > mineat)
                {
                    eatvalue = gamefield.players[eat].strength;
                    target = Pacman::left;
                }
                if (eatvalue == 0)
                {
                    double k1 = BfsValue(Pacman::FieldProp(myrow, (mycol + w - 1) % w), gamefield, myID)*bfs;
                    double k2 = FirstTakenBfs(Pacman::FieldProp(myrow, (mycol + w - 1) % w), gamefield, myID)*firstfruit;
                    double tempvalue = k1 + k2;
                    if (value < tempvalue && !IfEmpty && avoid != Pacman::left)
                    {
                        value = tempvalue; target = Pacman::left;
                    }
                }
            }
        eatenenemy.setfalse();
        return target;
    }
}


int main()
{
    string dict[] = {"WhyAreYouSoGoodAtIt", "WhyWouldItBLikeThis", "+++", "---"};
    string WhiteMembraneTauntLibrary[] = {"WB"};
    string randomloud = "random";
    Pacman::GameField gameField;

    // ����ڱ��ص��ԣ���input.txt����ȡ�ļ�������Ϊ����
    // �����ƽ̨�ϣ��򲻻�ȥ�������input.txt
    int myID = gameField.ReadInput("input.txt", QhHelper::data, QhHelper::globalData); // ���룬������Լ�ID
    srand(Pacman::seed + myID);

    //DFS
    //Helpers::FruitFirstPlay(gameField, myID, 0);


    // �����ǰ��Ϸ����״̬�Թ����ص��ԡ�ע���ύ��ƽ̨�ϻ��Զ��Ż��������ص��ġ�
    gameField.DebugPrint();
    Pacman::Direction ans = QhHelper::AIOutput(gameField, myID);

    // ��������Ƿ����
    if (ans != Pacman::stay) gameField.WriteOutput(ans, dict[Helpers::RandBetween(0, 4)], QhHelper::data, QhHelper::globalData);
    else
    {
        // ����������ĸ��������Ӯ�����
        for (int i = 0; i < 10000; i++)
            Helpers::RandomPlay(gameField, myID);

        int maxD = 0, d;
        for (d = 0; d < 5; d++)
            if (Helpers::actionScore[d] > Helpers::actionScore[maxD])
                maxD = d;
        gameField.WriteOutput((Pacman::Direction)(maxD - 1), randomloud, QhHelper::data, QhHelper::globalData);
    }
#ifndef _BOTZONE_ONLINE
    system("pause");
#endif
    return 0;
}