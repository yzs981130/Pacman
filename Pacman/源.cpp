/*
*  作者：叶志晟
* 【命名惯例】
*  r/R/y/Y：Row，行，纵坐标
*  c/C/x/X：Column，列，横坐标
*  数组的下标都是[y][x]或[r][c]的顺序
*  玩家编号0123
*
* 【坐标系】
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
* 【提示】你可以使用
* #ifndef _BOTZONE_ONLINE
* 这样的预编译指令来区分在线评测和本地评测
*
* 【提示】一般的文本编辑器都会支持将代码块折叠起来
* 如果你觉得自带代码太过冗长，可以考虑将整个namespace折叠
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
#define MAX_GENERATOR_COUNT 4 // 每个象限1
#define MAX_PLAYER_COUNT 4
#define MAX_TURN 100

// 你也可以选用 using namespace std; 但是会污染命名空间
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

// 平台提供的吃豆人相关逻辑处理程序
namespace Pacman
{
    const time_t seed = time(0);
    const int dx[] = {0, 1, 0, -1, 1, 1, -1, -1}, dy[] = {-1, 0, 1, 0, -1, 1, 1, -1};

    // 枚举定义；使用枚举虽然会浪费空间（sizeof(GridContentType) == 4），但是计算机处理32位的数字效率更高

    // 每个格子可能变化的内容，会采用“或”逻辑进行组合
    enum GridContentType
    {
        empty = 0, // 其实不会用到
        player1 = 1, // 1号玩家
        player2 = 2, // 2号玩家
        player3 = 4, // 3号玩家
        player4 = 8, // 4号玩家
        playerMask = 1 | 2 | 4 | 8, // 用于检查有没有玩家等
        smallFruit = 16, // 小豆子
        largeFruit = 32 // 大豆子
    };

    // 用玩家ID换取格子上玩家的二进制位
    GridContentType playerID2Mask[] = {player1, player2, player3, player4};
    string playerID2str[] = {"0", "1", "2", "3"};

    // 让枚举也可以用这些运算了（不加会编译错误）
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

    // 每个格子固定的东西，会采用“或”逻辑进行组合
    enum GridStaticType
    {
        emptyWall = 0, // 其实不会用到
        wallNorth = 1, // 北墙（纵坐标减少的方向）
        wallEast = 2, // 东墙（横坐标增加的方向）
        wallSouth = 4, // 南墙（纵坐标增加的方向）
        wallWest = 8, // 西墙（横坐标减少的方向）
        generator = 16 // 豆子产生器
    };

    // 用移动方向换取这个方向上阻挡着的墙的二进制位
    GridStaticType direction2OpposingWall[] = {wallNorth, wallEast, wallSouth, wallWest};

    // 方向，可以代入dx、dy数组，同时也可以作为玩家的动作
    enum Direction
    {
        stay = -1,
        up = 0,
        right = 1,
        down = 2,
        left = 3,
        // 下面的这几个只是为了产生器程序方便，不会实际用到
        ur = 4, // 右上
        dr = 5, // 右下
        dl = 6, // 左下
        ul = 7 // 左上
    };

    // 场地上带有坐标的物件
    struct FieldProp
    {
        int row, col;
        FieldProp(int r = 0, int c = 0):row(r), col(c) {}//缺省参数构造
    };

    // 场地上的玩家
    struct Player: FieldProp
    {
        int strength;
        int powerUpLeft;
        bool dead;
    };

    // 回合新产生的豆子的坐标
    struct NewFruits
    {
        FieldProp newFruits[MAX_GENERATOR_COUNT * 8];
        int newFruitCount;
    } newFruits[MAX_TURN];
    int newFruitsCount = 0;

    // 状态转移记录结构
    struct TurnStateTransfer
    {
        enum StatusChange // 可组合
        {
            none = 0,
            ateSmall = 1,
            ateLarge = 2,
            powerUpCancel = 4,
            die = 8,
            error = 16
        };

        // 玩家选定的动作
        Direction actions[MAX_PLAYER_COUNT];

        // 此回合该玩家的状态变化
        StatusChange change[MAX_PLAYER_COUNT];

        // 此回合该玩家的力量变化
        int strengthDelta[MAX_PLAYER_COUNT];
    };

    // 游戏主要逻辑处理类，包括输入输出、回合演算、状态转移，全局唯一
    class GameField
    {
    private:
        // 为了方便，大多数属性都不是private的

        // 记录每回合的变化（栈）
        TurnStateTransfer backtrack[MAX_TURN];

        // 这个对象是否已经创建
        static bool constructed;

    public:
        // 场地的长和宽
        int height, width;
        int generatorCount;
        int GENERATOR_INTERVAL, LARGE_FRUIT_DURATION, LARGE_FRUIT_ENHANCEMENT;

        // 场地格子固定的内容
        GridStaticType fieldStatic[FIELD_MAX_HEIGHT][FIELD_MAX_WIDTH];

        // 场地格子会变化的内容
        GridContentType fieldContent[FIELD_MAX_HEIGHT][FIELD_MAX_WIDTH];
        int generatorTurnLeft; // 多少回合后产生豆子
        int aliveCount; // 有多少玩家存活
        int smallFruitCount;
        int turnID;
        FieldProp generators[MAX_GENERATOR_COUNT]; // 有哪些豆子产生器
        Player players[MAX_PLAYER_COUNT]; // 有哪些玩家

                                          // 玩家选定的动作
        Direction actions[MAX_PLAYER_COUNT];

        // 恢复到上次场地状态。可以一路恢复到最开始。
        // 恢复失败（没有状态可恢复）返回false
        bool PopState()
        {
            if (turnID <= 0)
                return false;

            const TurnStateTransfer &bt = backtrack[--turnID];
            int i, _;

            // 倒着来恢复状态

            for (_ = 0; _ < MAX_PLAYER_COUNT; _++)
            {
                Player &_p = players[_];
                GridContentType &content = fieldContent[_p.row][_p.col];
                TurnStateTransfer::StatusChange change = bt.change[_];

                if (!_p.dead)
                {
                    // 5. 大豆回合恢复
                    if (_p.powerUpLeft || change & TurnStateTransfer::powerUpCancel)
                        _p.powerUpLeft++;

                    // 4. 吐出豆子
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

                // 2. 魂兮归来
                if (change & TurnStateTransfer::die)
                {
                    _p.dead = false;
                    aliveCount++;
                    content |= playerID2Mask[_];
                }

                // 1. 移形换影
                if (!_p.dead && bt.actions[_] != stay)
                {
                    fieldContent[_p.row][_p.col] &= ~playerID2Mask[_];
                    _p.row = (_p.row - dy[bt.actions[_]] + height) % height;
                    _p.col = (_p.col - dx[bt.actions[_]] + width) % width;
                    fieldContent[_p.row][_p.col] |= playerID2Mask[_];
                }

                // 0. 救赎不合法的灵魂
                if (change & TurnStateTransfer::error)
                {
                    _p.dead = false;
                    aliveCount++;
                    content |= playerID2Mask[_];
                }

                // *. 恢复力量
                if (!_p.dead)
                    _p.strength -= bt.strengthDelta[_];
            }

            // 3. 收回豆子
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

        // 判断指定玩家向指定方向移动是不是合法的（没有撞墙）
        inline bool ActionValid(int playerID, Direction &dir) const
        {
            if (dir == stay)
                return true;
            const Player &p = players[playerID];
            const GridStaticType &s = fieldStatic[p.row][p.col];
            return dir >= -1 && dir < 4 && !(s & direction2OpposingWall[dir]);
        }

        // 在向actions写入玩家动作后，演算下一回合局面，并记录之前所有的场地状态，可供日后恢复。
        // 是终局的话就返回false
        bool NextTurn()
        {
            int _, i, j;

            TurnStateTransfer &bt = backtrack[turnID];
            memset(&bt, 0, sizeof(bt));

            // 0. 杀死不合法输入
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
                        // 遇到比自己强♂壮的玩家是不能前进的
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

            // 1. 位置变化
            for (_ = 0; _ < MAX_PLAYER_COUNT; _++)
            {
                Player &_p = players[_];
                if (_p.dead)
                    continue;

                bt.actions[_] = actions[_];

                if (actions[_] == stay)
                    continue;

                // 移动
                fieldContent[_p.row][_p.col] &= ~playerID2Mask[_];
                _p.row = (_p.row + dy[actions[_]] + height) % height;
                _p.col = (_p.col + dx[actions[_]] + width) % width;
                fieldContent[_p.row][_p.col] |= playerID2Mask[_];
            }

            // 2. 玩家互殴
            for (_ = 0; _ < MAX_PLAYER_COUNT; _++)
            {
                Player &_p = players[_];
                if (_p.dead)
                    continue;

                // 判断是否有玩家在一起
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

                    // 这些玩家将会被杀死
                    int lootedStrength = 0;
                    for (i = begin; i < containedCount; i++)
                    {
                        int id = containedPlayers[i];
                        Player &p = players[id];

                        // 从格子上移走
                        fieldContent[p.row][p.col] &= ~playerID2Mask[id];
                        p.dead = true;
                        int drop = p.strength / 2;
                        bt.strengthDelta[id] += -drop;
                        bt.change[id] |= TurnStateTransfer::die;
                        lootedStrength += drop;
                        p.strength -= drop;
                        aliveCount--;
                    }

                    // 分配给其他玩家
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

            // 3. 产生豆子
            if (--generatorTurnLeft == 0)
            {
                generatorTurnLeft = GENERATOR_INTERVAL;
                NewFruits &fruits = newFruits[newFruitsCount++];
                fruits.newFruitCount = 0;
                for (i = 0; i < generatorCount; i++)
                    for (Direction d = up; d < 8; ++d)
                    {
                        // 取余，穿过场地边界
                        int r = (generators[i].row + dy[d] + height) % height, c = (generators[i].col + dx[d] + width) % width;
                        if (fieldStatic[r][c] & generator || fieldContent[r][c] & (smallFruit | largeFruit))
                            continue;
                        fieldContent[r][c] |= smallFruit;
                        fruits.newFruits[fruits.newFruitCount].row = r;
                        fruits.newFruits[fruits.newFruitCount++].col = c;
                        smallFruitCount++;
                    }
            }

            // 4. 吃掉豆子
            for (_ = 0; _ < MAX_PLAYER_COUNT; _++)
            {
                Player &_p = players[_];
                if (_p.dead)
                    continue;

                GridContentType &content = fieldContent[_p.row][_p.col];

                // 只有在格子上只有自己的时候才能吃掉豆子
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

            // 5. 大豆回合减少
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

            // 是否只剩一人？
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

            // 是否回合超限？
            if (turnID >= 100)
                return false;

            return true;
        }

        // 读取并解析程序输入，本地调试或提交平台使用都可以。
        // 如果在本地调试，程序会先试着读取参数中指定的文件作为输入文件，失败后再选择等待用户直接输入。
        // 本地调试时可以接受多行以便操作，Windows下可以用 Ctrl-Z 或一个【空行+回车】表示输入结束，但是在线评测只需接受单行即可。
        // localFileName 可以为NULL
        // obtainedData 会输出自己上回合存储供本回合使用的数据
        // obtainedGlobalData 会输出自己的 Bot 上以前存储的数据
        // 返回值是自己的 playerID
        int ReadInput(const char *localFileName, string &obtainedData, string &obtainedGlobalData)
        {
            string str, chunk;
#ifdef _BOTZONE_ONLINE
            std::ios::sync_with_stdio(false); //ω\\)
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

            // 读取场地静态状况
            Json::Value field = input["requests"][(Json::Value::UInt) 0],
                staticField = field["static"], // 墙面和产生器
                contentField = field["content"]; // 豆子和玩家
            height = field["height"].asInt();
            width = field["width"].asInt();
            LARGE_FRUIT_DURATION = field["LARGE_FRUIT_DURATION"].asInt();
            LARGE_FRUIT_ENHANCEMENT = field["LARGE_FRUIT_ENHANCEMENT"].asInt();
            generatorTurnLeft = GENERATOR_INTERVAL = field["GENERATOR_INTERVAL"].asInt();

            PrepareInitialField(staticField, contentField);

            // 根据历史恢复局面
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

        // 根据 static 和 content 数组准备场地的初始状况
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

        // 完成决策，输出结果。
        // action 表示本回合的移动方向，stay 为不移动
        // tauntText 表示想要叫嚣的言语，可以是任意字符串，除了显示在屏幕上不会有任何作用，留空表示不叫嚣
        // data 表示自己想存储供下一回合使用的数据，留空表示删除
        // globalData 表示自己想存储供以后使用的数据（替换），这个数据可以跨对局使用，会一直绑定在这个 Bot 上，留空表示删除
        void WriteOutput(Direction action, string tauntText = "", string data = "", string globalData = "") const
        {
            Json::Value ret;
            ret["response"]["action"] = action;
            ret["response"]["tauntText"] = tauntText;
            ret["data"] = data;
            ret["globaldata"] = globalData;
            ret["debug"] = (Json::Int)seed;

#ifdef _BOTZONE_ONLINE
            Json::FastWriter writer; // 在线评测的话能用就行……
#else
            Json::StyledWriter writer; // 本地调试这样好看 > <
#endif
            cout << writer.write(ret) << endl;
        }

        // 用于显示当前游戏状态，调试用。
        // 提交到平台后会被优化掉。
        inline void DebugPrint() const
        {
#ifndef _BOTZONE_ONLINE
            printf("回合号【%d】存活人数【%d】| 图例 产生器[G] 有玩家[0/1/2/3] 多个玩家[*] 大豆[o] 小豆[.]\n", turnID, aliveCount);
            for (int _ = 0; _ < MAX_PLAYER_COUNT; _++)
            {
                const Player &p = players[_];
                printf("[玩家%d(%d, %d)|力量%d|加成剩余回合%d|%s]\n",
                    _, p.row, p.col, p.strength, p.powerUpLeft, p.dead ? "死亡" : "存活");
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

        // 初始化游戏管理器
        GameField()
        {
            if (constructed)
                throw runtime_error("请不要再创建 GameField 对象了，整个程序中只应该有一个对象");
            constructed = true;

            turnID = 0;
        }

        GameField(const GameField &b): GameField() {}
    };

    bool GameField::constructed = false;
}

// 一些辅助程序
namespace Helpers
{

    int actionScore[5] = {};
    int TempMaxValue = -99999, TempValue = 0;
    Pacman::Direction TempAction[5];
    Pacman::Direction MyDFSAct;
    Pacman::FieldProp BeginPosition;

    //重载==供bfs用
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
            // 对每个玩家生成随机的合法动作
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

            // 演算一步局面变化
            // NextTurn返回true表示游戏没有结束
            bool hasNext = gameField.NextTurn();
            count++;

            if (!hasNext)
                break;
        }

        // 计算分数
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

        // 恢复游戏状态到最初（就是本回合）
        while (count-- > 0)
            gameField.PopState();
    }

    int FindShortestPath(const Pacman::GameField & gamefield, const Pacman::FieldProp & Begin, const Pacman::FieldProp & End)
    {
        int h = gamefield.height; int w = gamefield.width;//获取height width
        if (Begin.row >= h || Begin.row < 0 || Begin.col >= w || Begin.col < 0)
            throw runtime_error("输入坐标不合法");
        if (End.row >= h || End.row < 0 || End.col >= w || End.col < 0)
            throw runtime_error("输入坐标不合法");
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
                //加入敌人预警
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
    string data, globalData; // 这是回合之间可以传递的信息
    stringstream ss;
    Pacman::Direction avoid = Pacman::stay;
    string tmpGlobalData;
    bool OffRoadPosition[FIELD_MAX_HEIGHT][FIELD_MAX_WIDTH] = {0};

    int forwardsteps[6][2];
    const double stopnumber = 999999;//赋值的无穷，使路被封堵
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
    //code 中 value 类型一律采用double
    //r,c为bfs开始坐标，value是估值变量的引用，用于传递给op，op则是对应的具体的操作函数
    //op返回的是bool值 返回false代表bfs结束，返回true代表bfs进行
    //r,c为bfs开始坐标，value是估值变量的引用，用于传递给op，op则是对应的具体的操作函数
    //op返回的是bool值 返回false代表bfs结束，返回true代表bfs进行
    bool IfEmpty;     //用于判断搜索范围之内是否毫无fruit 搜索value=0且所有点为零
                      //false 代表 不空 true代表 空

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
    //return 99999 代表两点不可连接
    //下列是求最短路径的
    int Terr, Terc;//用来暂时储存Begin の row 和 col的全局var
                   //最短路径bfsop
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
        Terr = End.row; Terc = End.col;//初始化
        double value = stopnumber;//开始赋值为不通
        BfsSearch(gamefield, Begin.row, Begin.col, value, FindShortestPathBfsop, -1);//myID赋值-1无作用																			 
        return value;
    }


    //判断是否存在死路且无fruit
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
    Eaten eatenenemy;//记录准备吃的敌人的最远可能路程
                     /*
                     fruitthink 1 考虑fruit遇到fruit返回true 0 同时记录吃人的一些信息
                     */
    bool OffRoad(const Pacman::GameField& gamefield, Pacman::Direction d, int myID, int fruitthink, int Depth = 6)//x前进方向,Depth 为搜索深度
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
        if (!fruitthink)//判断会不会被逼近死路时将比自己力量强的敌人周围设成路障
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
    //判断自己是否有可能被敌人追进死路吃掉
    //是的话打死也不能朝这个方向走呗
    //如果是返回true else false
    //走进死路吃人的函数
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
    //搜索一定步数内是否有危险敌人存在
    struct GetFruit:Pacman::FieldProp
    {
        Pacman::GridContentType fruit;//种类
        int path;//路径
        GetFruit(Pacman::GridContentType a, int b, int r, int c):fruit(a), path(b), Pacman::FieldProp(r, c) {}
    };
    //抓取最近的一个fruit smallfruit和largefruit分开计算
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
    //判断敌人的威胁性
    //false 没威胁 true  有威胁
    bool enemythreat(const Pacman::GameField& gamefield, int k, int myID, int r, int c, GetFruit & target)//k是enemy对应编号
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
    //返回搜索到的敌人编号 
    //-1就是没找到
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

    //返回true 有威胁 返回false 无威胁
    bool LifeSaveEnemy(Pacman::Direction d, const Pacman::GameField& gamefield, int myID)
    {
        GetFruit sfruit = GetFruitPath(d, gamefield, myID, Pacman::smallFruit),
            lfruit = GetFruitPath(d, gamefield, myID, Pacman::largeFruit);
        int person1 = SearchEnemy(gamefield, myID, sfruit),
            person2 = SearchEnemy(gamefield, myID, lfruit);
        if (person1 == -1 && person2 == -1) return false;
        else return true;
    }

    //1.判断是否进入无用的死胡同
    //2.判断是否进入死胡同后会被艹
    //3.返回true代表可以走 ，返回false代表不能走
    bool LifeSave(Pacman::Direction d, const Pacman::GameField& gamefield, int myID)
    {
        int r = (myrow + h + Pacman::dy[d]) % h;
        int c = (mycol + w + Pacman::dx[d]) % w;
        bool del1 = OffRoad(gamefield, d, myID, 1);//false 有豆子或者通 true 无豆子且不通
        bool del2 = OffRoad(gamefield, d, myID, 0);//true 不通 false 通 顺带对eatenenemy进行赋值
        bool del3 = LifeSaveEnemy(d, gamefield, myID);//true有威胁 false无威胁
        if (del2)
        {
            if (del3) return false;
            if (!del1) return true;
            if (eatenenemy) return true;
            return false;
        }
        else return true;
    }

    //攫取价值
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
        double largevalue = 0;//why 不用const ?因为后面要赋值
        if (mypowerUpLeft > 3)
            largevalue = 10;
        //largevalue = 0.12*(gamefield.LARGE_FRUIT_DURATION - mypowerUpLeft);
        else
            largevalue = 100;
        //const int generatvalue = 5;
        const double threat = -40;
        //const double position = -5;
        const double sFruitofsEnemy = 0;//敌人距离近时弱敌人位置对果子估值影响参数
        const double sFruitoflEnemy = 0;//敌人距离近时强敌人位置对果子估值影响参数
        const double lFruitoflEnemy = 0;//敌人距离远时强敌人位置对果子估值函数影响
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
        // 如果 比你威力强的敌人 在两步及之内 说啥 快跑！
        //player.dead false玩家存活
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
    //BfsSearch_的op函数
    inline bool BfsValueop(const Pacman::GameField &gamefield, int r, int c, double &value, int path, int myID)
    {
        const int Maxstep = 20;
        if (path > Maxstep) return false;
        //if (r == myrow && c == mycol) return true;
        double pointvalue = ValueTake(gamefield, r, c, myID, path);//返回该点对出发点的估值
        double pointvalue2 = ValueTake2(gamefield, r, c, myID, path);//对generator附近位置的估值
        value += pointvalue*(Maxstep - path);
        value += pointvalue2;
        if (pointvalue != 0 || pointvalue2 != 0) IfEmpty = false;
        return true;
    }
    double BfsValue(Pacman::FieldProp Begin, const Pacman::GameField & gamefield, int myID)
    {
        IfEmpty = true;//reset variable  IfEmpty
        double value = 0;//函数返回的价值,等于估值乘以距离 
        BfsSearch(gamefield, Begin.row, Begin.col, value, BfsValueop, myID);
        return value;
    }
    //BFS探索有限步
    //局部贪心 走最近的果子

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
    //返回离该点最近不属的果子
    int FirstTakenBfs(Pacman::FieldProp Begin, const Pacman::GameField & gamefield, int myID)
    {
        double value = 0;
        BfsSearch(gamefield, Begin.row, Begin.col, value, FirstTakenop, myID);
        return value;
    }

    //判断威胁是否近在眼前
    //是的话打死也不能朝这个方向走呗
    //如果是返回true else false
    //控制总的输入输出
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
            if (LifeSave(Pacman::up, gamefield, myID))//判断不是死路且没有生命危险
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

    // 如果在本地调试，有input.txt则会读取文件内容作为输入
    // 如果在平台上，则不会去检查有无input.txt
    int myID = gameField.ReadInput("input.txt", QhHelper::data, QhHelper::globalData); // 输入，并获得自己ID
    srand(Pacman::seed + myID);

    //DFS
    //Helpers::FruitFirstPlay(gameField, myID, 0);


    // 输出当前游戏局面状态以供本地调试。注意提交到平台上会自动优化掉，不必担心。
    gameField.DebugPrint();
    Pacman::Direction ans = QhHelper::AIOutput(gameField, myID);

    // 随机决定是否叫嚣
    if (ans != Pacman::stay) gameField.WriteOutput(ans, dict[Helpers::RandBetween(0, 4)], QhHelper::data, QhHelper::globalData);
    else
    {
        // 简单随机，看哪个动作随机赢得最多
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