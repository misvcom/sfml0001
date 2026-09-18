#include <bits/stdc++.h>
#include <SFML/Graphics.hpp>
#include <SFML/System/Clock.hpp>
#include <SFML/System/Time.hpp>

// ==================== 游戏常量配置 ====================
// 窗口尺寸（单位：像素）
constexpr unsigned int WINDOW_WIDTH = 800;
constexpr unsigned int WINDOW_HEIGHT = 600;

// 挡板参数：宽度、高度、移动速度（像素/秒）
constexpr float PADDLE_WIDTH = 100.0f;
constexpr float PADDLE_HEIGHT = 20.0f;
constexpr float PADDLE_SPEED = 500.0f;

// 球的参数：半径、移动速度（像素/秒）
constexpr float BALL_RADIUS = 10.0f;
constexpr float BALL_SPEED = 300.0f;

// 砖块参数：宽度、高度、行列数
constexpr float BRICK_WIDTH = 70.0f;
constexpr float BRICK_HEIGHT = 25.0f;
constexpr int BRICK_COLS = 10;  // 每行砖块数量
constexpr int BRICK_ROWS = 5;   // 砖块行数

// ==================== 方向枚举 ====================
// 定义挡板的移动方向
enum class Direction { LEFT, RIGHT, NONE };

// ==================== 砖块结构体 ====================
// Brick 代表游戏中可被击碎的砖块
struct Brick {
    sf::RectangleShape shape;  // SFML 矩形形状，用于渲染
    bool active = true;        // 标记砖块是否仍然存在

    // 构造函数：在指定位置创建指定颜色的砖块
    Brick(float x, float y, const sf::Color& color) {
        shape.setSize({ BRICK_WIDTH, BRICK_HEIGHT });
        shape.setFillColor(color);              // 填充颜色
        shape.setOutlineColor(sf::Color::Black); // 边框颜色（黑色）
        shape.setOutlineThickness(1.0f);        // 边框粗细（1像素）
        shape.setPosition({ x, y });            // 设置初始位置
    }
};

// ==================== 挡板结构体 ====================
// Paddle 代表玩家控制的底部挡板
struct Paddle {
    sf::RectangleShape shape;      // SFML 矩形形状，用于渲染
    Direction direction = Direction::NONE;  // 当前移动方向

    // 构造函数：将挡板放置在窗口底部中央
    Paddle() {
        shape.setSize({ PADDLE_WIDTH, PADDLE_HEIGHT });
        shape.setFillColor(sf::Color::White);
        // 挡板初始位置：水平居中，底部留 30 像素间距
        shape.setPosition({
            (WINDOW_WIDTH - PADDLE_WIDTH) / 2.0f,
            WINDOW_HEIGHT - PADDLE_HEIGHT - 30.0f
        });
    }

    // 根据方向和帧时间 dt（秒）更新挡板位置
    void update(float dt) {
        if (direction == Direction::LEFT) {
            auto pos = shape.getPosition();
            pos.x -= PADDLE_SPEED * dt;  // 向左移动
            if (pos.x < 0) pos.x = 0;    // 限制左边界
            shape.setPosition(pos);
        }
        else if (direction == Direction::RIGHT) {
            auto pos = shape.getPosition();
            pos.x += PADDLE_SPEED * dt;  // 向右移动
            if (pos.x + PADDLE_WIDTH > WINDOW_WIDTH)  // 限制右边界
                pos.x = WINDOW_WIDTH - PADDLE_WIDTH;
            shape.setPosition(pos);
        }
    }
};

// ==================== 球结构体 ====================
// Ball 代表游戏中弹跳的小球
struct Ball {
    sf::CircleShape shape;      // SFML 圆形形状，用于渲染
    // velocity: 球的速度向量 (vx, vy)，单位：像素/秒
    //   vx > 0 → 向右运动，vx < 0 → 向左运动
    //   vy > 0 → 向下运动，vy < 0 → 向上运动（屏幕坐标系 Y 向下为正）
    //   速度大小 = √(vx² + vy²)，决定球的实际移动速度
    sf::Vector2f velocity;
    bool attached = true;       // true = 球附着在挡板上，尚未发射

    // 构造函数：初始化球的外观
    Ball() {
        shape.setRadius(BALL_RADIUS);
        shape.setFillColor(sf::Color::Cyan);  // 青色球
        shape.setOrigin({ BALL_RADIUS, BALL_RADIUS });  // 设置圆心为原点（用于旋转/缩放）
    }

    // 重置球的状态：将球放回挡板上方
    // 参数 paddleCenter：挡板中心正上方的目标位置
    //   位置公式：ball_pos = paddleCenter = (paddle.x + width/2, paddle.y - radius)
    void reset(const sf::Vector2f& paddleCenter) {
        shape.setPosition(paddleCenter);  // 将球设置到挡板上方
        velocity = { 0, 0 };              // 速度归零（静止）
        attached = true;                  // 重新附着到挡板
    }

    // 发射球：从挡板脱离，随机向上方发射
    void launch() {
        if (!attached) return;  // 已经发射则不处理
        attached = false;       // 标记为已发射
        // 随机生成水平方向的偏移量（-1.0 到 1.0 之间）
        srand(static_cast<unsigned>(time(nullptr)));
        float xDir = (rand() % 200 - 100) / 100.0f;
        // 发射公式：设置速度向量 (vx, vy)
        //   vx = xDir * BALL_SPEED，范围 [-BALL_SPEED, +BALL_SPEED]
        //   vy = -BALL_SPEED，始终向上（Y 轴负方向）
        // 速度大小 = √((xDir*BALL_SPEED)² + (-BALL_SPEED)²)
        //          = BALL_SPEED * √(xDir² + 1)
        // 当 xDir=0 时，速度=BALL_SPEED=300 px/s（纯垂直向上）
        // 当 xDir=±1 时，速度=√2*BALL_SPEED≈424 px/s（45度斜向）
        velocity = { xDir * BALL_SPEED, -BALL_SPEED };
    }

    // 更新球的位置，处理与边界的碰撞
    void update(float dt, Paddle& paddle) {
        if (attached) {
            // ===== 球跟随挡板（未发射状态）=====
            // 球位置 = 挡板中心正上方
            //   ball.x = paddle.x + PADDLE_WIDTH/2  （挡板中心 X 坐标）
            //   ball.y = paddle.y - BALL_RADIUS     （挡板上方，刚好接触）
            auto paddleCenter = paddle.shape.getPosition();
            paddleCenter.x += PADDLE_WIDTH / 2.0f;  // 挡板中心 X = 左边缘 + 半宽
            paddleCenter.y -= BALL_RADIUS;           // 球底部刚好接触挡板顶部
            shape.setPosition(paddleCenter);
            return;
        }

        // 根据速度和时间更新位置
        // 位移公式：Δs = v × Δt  （位移 = 速度 × 时间）
        //   新位置 = 原位置 + 速度向量 × 帧时间
        //   pos.x(t+Δt) = pos.x(t) + vx × dt
        //   pos.y(t+Δt) = pos.y(t) + vy × dt
        auto pos = shape.getPosition();
        pos.x += velocity.x * dt;
        pos.y += velocity.y * dt;

        // 左右边界反弹：碰到左右墙壁时反转水平速度
        // 碰撞条件：球心到边界的距离 ≤ 球半径（即 pos.x - r ≤ 0 或 pos.x + r ≥ 宽度）
        // 反射公式（水平方向）：vx' = -vx  （入射角 = 反射角）
        // 物理原理：垂直于边界的速度分量反转，平行分量不变
        //  clamp 防止球因高速穿过边界而卡住
        if (pos.x - BALL_RADIUS <= 0.0f || pos.x + BALL_RADIUS >= WINDOW_WIDTH) {
            velocity.x = -velocity.x;  // 水平速度取反：向右 → 向左
            pos.x = std::clamp(pos.x, BALL_RADIUS, static_cast<float>(WINDOW_WIDTH) - BALL_RADIUS);
        }
        // 顶部边界反弹：碰到天花板时反转垂直速度
        // 碰撞条件：球心到顶部边界的距离 ≤ 球半径（pos.y - r ≤ 0）
        // 反射公式（垂直方向）：vy' = -vy  （入射角 = 反射角）
        if (pos.y - BALL_RADIUS <= 0.0f) {
            velocity.y = -velocity.y;  // 垂直速度取反：向下 → 向上
            pos.y = BALL_RADIUS;       // 将球校正到刚好接触边界的位置
        }
        // 底部检测：球掉出屏幕底部则重置（游戏惩罚机制）
        // 碰撞条件：球顶部超过窗口底部（pos.y - r > WINDOW_HEIGHT）
        // 此时不做反弹，直接重置球到挡板上方
        if (pos.y - BALL_RADIUS > WINDOW_HEIGHT) {
            reset({ paddle.shape.getPosition().x + PADDLE_WIDTH / 2.0f,
                    paddle.shape.getPosition().y - BALL_RADIUS });
            return;
        }

        shape.setPosition(pos);
    }
};

// ==================== 碰撞检测 ====================
// 判断两个矩形是否相交（AABB 碰撞检测）
// SFML 3.x 使用 findIntersection 返回可选的交集区域
bool intersects(const sf::FloatRect& a, const sf::FloatRect& b) {
    return a.findIntersection(b).has_value();
}

// ==================== 主函数 ====================
int main()
{
    // 创建 SFML 渲染窗口：800x600 像素，标题为"SFML Breakout Demo"
    // sf::Style::Close 表示窗口只显示关闭按钮
    sf::RenderWindow window(
        sf::VideoMode({ WINDOW_WIDTH, WINDOW_HEIGHT }),
        "SFML Breakout Demo - 按空格键发射",
        sf::Style::Close
    );
    window.setFramerateLimit(60);  // 限制帧率为 60 FPS

    // 初始化游戏对象
    Paddle paddle;  // 创建挡板

    Ball ball;      // 创建球
    // 将球放置在挡板上方中央位置
    ball.reset({ paddle.shape.getPosition().x + PADDLE_WIDTH / 2.0f,
                 paddle.shape.getPosition().y - BALL_RADIUS });

    // 初始化砖块：创建 BRICK_ROWS 行 x BRICK_COLS 列的砖块阵列
    std::vector<Brick> bricks;
    // 为每一行定义不同的颜色（彩虹色）
    std::vector<sf::Color> brickColors = {
        sf::Color::Red, sf::Color(255, 165, 0), sf::Color::Yellow,
        sf::Color::Green, sf::Color::Blue
    };
    // 计算砖块阵列的起始 X 坐标，使其水平居中
    float startX = (WINDOW_WIDTH - (BRICK_COLS * BRICK_WIDTH)) / 2.0f;
    for (int row = 0; row < BRICK_ROWS; ++row) {
        for (int col = 0; col < BRICK_COLS; ++col) {
            float x = startX + col * BRICK_WIDTH;   // 当前砖块的 X 坐标
            float y = 50.0f + row * BRICK_HEIGHT;   // 当前砖块的 Y 坐标（顶部留 50 像素）
            bricks.emplace_back(x, y, brickColors[row % brickColors.size()]);
        }
    }

    // 游戏状态变量
    int score = 0;           // 当前得分
    bool gameWon = false;    // 是否已获胜（所有砖块被击碎）

    // SFML 时钟：用于计算每帧之间的时间差（delta time）
    sf::Clock clock;

    // ==================== 主游戏循环 ====================
    while (window.isOpen())
    {
        // 获取上一帧到当前帧的时间（秒），用于平滑的基于时间的运动
        float dt = clock.restart().asSeconds();

        // ----- 事件处理 -----
        while (const std::optional event = window.pollEvent())
        {
            // 关闭窗口事件
            if (event->is<sf::Event::Closed>())
                window.close();

            // 键盘按下事件
            if (event->is<sf::Event::KeyPressed>()) {
                // 空格键：发射球
                if (event->getIf<sf::Event::KeyPressed>()->code == sf::Keyboard::Key::Space) {
                    ball.launch();
                }
                // R 键：重置游戏
                if (event->getIf<sf::Event::KeyPressed>()->code == sf::Keyboard::Key::R) {
                    score = 0;
                    gameWon = false;
                    paddle = Paddle();  // 重新创建挡板
                    ball.reset({ paddle.shape.getPosition().x + PADDLE_WIDTH / 2.0f,
                                 paddle.shape.getPosition().y - BALL_RADIUS });
                    for (auto& brick : bricks) {
                        brick.active = true;  // 恢复所有砖块
                    }
                }
            }
        }

        // ----- 实时输入检测 -----
        // 检测 A/D 或左右方向键来控制挡板
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A) ||
            sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left)) {
            paddle.direction = Direction::LEFT;
        }
        else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D) ||
                 sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right)) {
            paddle.direction = Direction::RIGHT;
        }
        else {
            paddle.direction = Direction::NONE;
        }

        // ----- 更新游戏逻辑 -----
        if (!gameWon) {
            paddle.update(dt);       // 更新挡板位置
            ball.update(dt, paddle); // 更新球的位置

            // 球与挡板碰撞检测（仅在球向下运动时检测）
            if (!ball.attached) {
                auto ballBounds = ball.shape.getGlobalBounds();
                auto paddleBounds = paddle.shape.getGlobalBounds();
                if (ball.velocity.y > 0 && intersects(ballBounds, paddleBounds)) {
                    // ===== 挡板反弹物理模型 =====
                    // 假设：挡板中心为坐标原点，建立一维坐标系
                    //
                    //   挡板左端 (-R)         中心 (0)         挡板右端 (+R)
                    //      ←───────●───────────────────●───────→
                    //             -R=0                +R=50
                    //
                    //  hitPoint = ball.x - paddle_center.x  （球相对于挡板中心的水平偏移）
                    //  范围：[-PADDLE_WIDTH/2, +PADDLE_WIDTH/2] = [-50, +50]
                    //
                    //  normalizedHit = hitPoint / (PADDLE_WIDTH/2)  （归一化到 [-1, 1]）
                    //
                    //  新速度计算：
                    //    vx' = normalizedHit × BALL_SPEED  （最大 ±300 px/s）
                    //    vy' = -|vy|  （确保向上反弹，取绝对值后取反）
                    //
                    //  示例（BALL_SPEED = 300）：
                    //    击中挡板中心：hitPoint=0   → vx'=0,    vy'=-300  （垂直向上）
                    //    击中挡板右端：hitPoint=+50 → vx'=+300, vy'=-300  （45°右上方）
                    //    击中挡板左端：hitPoint=-50 → vx'=-300, vy'=-300  （45°左上方）
                    float hitPoint = ball.shape.getPosition().x -
                                     (paddle.shape.getPosition().x + PADDLE_WIDTH / 2.0f);
                    float normalizedHit = hitPoint / (PADDLE_WIDTH / 2.0f);
                    ball.velocity.x = normalizedHit * BALL_SPEED;     // 水平速度：-300 ~ +300
                    ball.velocity.y = -std::abs(ball.velocity.y);       // 垂直速度：确保向上（负值）
                }

                // ===== 球与砖块碰撞检测 =====
                // 碰撞处理：假设球以一定角度撞击砖块的某一边
                // 简化模型：无论球从哪个方向撞击砖块，都只反转垂直速度 vy
                // 这假设砖块是水平排列的，球主要从上方或下方撞击
                //
                // 反射公式：vy' = -vy  （垂直速度取反，水平速度不变）
                // 实际物理中，应根据碰撞面的法线方向进行向量反射：
                //   v' = v - 2(v·n)n   （n 为碰撞面单位法向量）
                // 但为简化游戏逻辑，采用简化的垂直反弹
                for (auto& brick : bricks) {
                    if (brick.active && intersects(ball.shape.getGlobalBounds(), brick.shape.getGlobalBounds())) {
                        brick.active = false;               // 标记砖块被击碎
                        ball.velocity.y = -ball.velocity.y; // 垂直速度取反（反弹）
                        score += 10;                        // 增加得分
                        break;                              // 一帧只处理一次碰撞，避免多次反弹
                    }
                }

                // 检查是否获胜：遍历所有砖块，如果还有活动的砖块则未获胜
                gameWon = true;
                for (const auto& brick : bricks) {
                    if (brick.active) {
                        gameWon = false;
                        break;
                    }
                }
            }
        }

        // ----- 绘制阶段 -----
        window.clear(sf::Color::Black);  // 清空画布，填充黑色背景

        // 绘制所有激活的砖块
        for (const auto& brick : bricks) {
            if (brick.active) {
                window.draw(brick.shape);
            }
        }

        // 绘制挡板和球
        window.draw(paddle.shape);
        window.draw(ball.shape);

        // 显示渲染结果（双缓冲交换）
        window.display();
    }

    return 0;
}
