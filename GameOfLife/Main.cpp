// Game of life（ライフゲーム）
//https://ja.wikipedia.org/wiki/ライフゲーム


#include "stdafx.h"

// 1 セルが 1 バイトになるよう、ビットフィールドを使用
struct Cell
{
	bool previous : 1;
	bool current : 1;
};

/*
// フィールドをランダムなセル値で埋める関数
// @param[out]  grid ランダム値で埋めたセル
*/
static void FillRandom(Grid<Cell>& grid);

/*
// フィールドの状態を更新する関数
// 誕生：死んでいるセルに隣接する生きたセルがちょうど3つあれば、次の世代が誕生する。
// 生存：生きているセルに隣接する生きたセルが2つか3つならば、次の世代でも生存する。
// 過疎：生きているセルに隣接する生きたセルが1つ以下ならば、過疎により死滅する。
// 過密：生きているセルに隣接する生きたセルが4つ以上ならば、過密により死滅する。
// @param[out] grid 更新後のセル
*/
static void Update(Grid<Cell>& grid);

/*
// フィールドの状態を画像化する関数
// @param[in] grid セルの状態
// @param[out] image 更新後フィールドの画像
*/
static void CopyToImage(const Grid<Cell>& grid, Image& image);

void Main()
{
	Window::SetTitle(U"ライフゲーム");
	Window::Resize(840, 600);	// 840 x 600
	constexpr int32 fieldWidth = 60;	// フィールドのセルの数（横）
	constexpr int32 fieldHeight = 60;	// フィールドのセルの数（縦）
	bool autoStep = false;		// 自動再生
	double speed = 0.5;			// 更新頻度
	bool doesShowGrid = true;	// グリッドの表示
	bool doesUpdate = false;	// 画像の更新の必要があるか

	// 計算をしない境界部分も含めたサイズで二次元配列を確保
	Grid<Cell> grid(fieldWidth + 2, fieldHeight + 2, Cell{ 0,0 });

	// フィールドの状態を可視化するための画像
	Image image(fieldWidth, fieldHeight, Palette::Black);

	// 動的テクスチャ
	DynamicTexture texture(image);

	Stopwatch s(StartImmediately::Yes);

	while (System::Update())
	{
		// フィールドをランダムな値で埋めるボタン
		if (SimpleGUI::ButtonAt(U"ランダム", Vec2(720, 40), 200))
		{
			FillRandom(grid);
			doesUpdate = true;
		}

		// フィールドのセルをすべてゼロにするボタン
		if (SimpleGUI::ButtonAt(U"クリア", Vec2(720, 80), 200))
		{
			grid.fill({ 0, 0 });
			doesUpdate = true;
		}

		// 一時停止 / 再生ボタン
		if (SimpleGUI::ButtonAt(autoStep ? U"中断 ■" : U"再生 ▶", Vec2(720, 160), 200))
		{
			autoStep = !autoStep;
		}

		// 更新頻度変更スライダー
		SimpleGUI::SliderAt(U"更新速度", speed, 1.0, 0.1, Vec2(720, 200), 80, 120);

		// 1ステップ進めるボタン、または更新タイミングの確認
		if (SimpleGUI::ButtonAt(U"更新", Vec2(720, 240), 200) || (autoStep && s.sF() >= (speed * speed)))
		{
			Update(grid);
			doesUpdate = true;
			s.restart();
		}

		// グリッド表示の有無を指定するチェックボックス
		SimpleGUI::CheckBoxAt(doesShowGrid, U"グリッド", Vec2(720, 320), 200);

		// フィールド上でのセルの編集
		if (Rect(0, 0, 599).mouseOver())
		{
			const Point target = Cursor::Pos() / 10 + Point(1, 1);

			if (MouseL.pressed())
			{
				grid[target].current = true;
				doesUpdate = true;
			}
			else if (MouseR.pressed())
			{
				grid[target].current = false;
				doesUpdate = true;
			}
		}

		// 画像の更新
		if (doesUpdate)
		{
			CopyToImage(grid, image);
			texture.fill(image);
			doesUpdate = false;
		}

		// 画像をフィルタなしで拡大して表示
		{
			ScopedRenderStates2D sampler(SamplerState::ClampNearest);
			texture.scaled(10).draw();
		}

		// グリッドの表示
		if (doesShowGrid)
		{
			for (auto i : step(61))
			{
				Rect(0, i * 10, 600, 1).draw(ColorF(0.4));
				Rect(i * 10, 0, 1, 600).draw(ColorF(0.4));
			}
		}

		if (Rect(0, 0, 599).mouseOver())
		{
			Cursor::RequestStyle(CursorStyle::Hidden);
			Rect(Cursor::Pos() / 10 * 10, 10).draw(Palette::Orange);
		}
	}
}

void FillRandom(Grid<Cell>& grid)
{
	grid.fill({ 0,0 });

	// 境界のセルを除いて更新
	for (auto y : Range(1, grid.height() - 2))
	{
		for (auto x : Range(1, grid.width() - 2))
		{
			grid[y][x] = { /*previous=*/0, /*current=*/RandomBool(0.5)};
		}
	}
}

void Update(Grid<Cell>& grid)
{
	for (auto& cell : grid)
	{
		cell.previous = cell.current;
	}

	// 境界のセルを除いて更新
	for (auto y : Range(1, grid.height() - 2))
	{
		for (auto x : Range(1, grid.width() - 2))
		{
			const int32 centerCell = grid[y][x].previous;

			int32 numLivingCell = 0;	// 近傍のセルの生存数
			numLivingCell += grid[y - 1][x - 1].previous;
			numLivingCell += grid[y - 1][x].previous;
			numLivingCell += grid[y - 1][x + 1].previous;
			numLivingCell += grid[y][x - 1].previous;
			numLivingCell += grid[y][x + 1].previous;
			numLivingCell += grid[y + 1][x - 1].previous;
			numLivingCell += grid[y + 1][x].previous;
			numLivingCell += grid[y + 1][x + 1].previous;

			// セルの状態の更新
			grid[y][x].current =
				(centerCell == 0 && numLivingCell == 3)	// 誕生
				|| (centerCell == 1 && (numLivingCell == 2 || numLivingCell == 3));	//生存
		}
	}
}

void CopyToImage(const Grid<Cell>& grid, Image& image)
{
	static const Color colorLivingCell(0, 255, 0);
	static const Color colorDeadCell(Palette::Black);

	for (auto y : step(image.height()))
	{
		for (auto x : step(image.width()))
		{
			image[y][x] = grid[y + 1][x + 1].current ? colorLivingCell : colorDeadCell;
		}
	}
}
