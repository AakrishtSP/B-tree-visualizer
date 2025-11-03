
#include <raylib.h>
#include <string>
#include <sstream>
#include <cmath>
#include <algorithm>
#include <unordered_map>
#include <cfloat>
#include "btree.hpp"

int main() {
	const int screenWidth = 1200;
	const int screenHeight = 800;
	InitWindow(screenWidth, screenHeight, "B-Tree Visualizer");
	SetTargetFPS(60);

	BTree tree(3);
	
	for (int i = 0; i < 8; ++i) tree.insert(10 + rand() % 90);

	Vector2 pan = {0, 0};
	float zoom = 1.0f;
	bool dragging = false;
	Vector2 lastMouse = {0, 0};

	int nextRandom = 100;
	int hoveredKey = -1;
	
	enum class TypingMode { None, Insert, Multi };
	TypingMode typingMode = TypingMode::None;
	bool typing = false;
	std::string typed = "";

	auto fitView = [&](const std::vector<float>& xs, const std::vector<float>& ys){
		if (xs.empty() || ys.empty()) return;
		float minx = *std::min_element(xs.begin(), xs.end());
		float maxx = *std::max_element(xs.begin(), xs.end());
		float miny = *std::min_element(ys.begin(), ys.end());
		float maxy = *std::max_element(ys.begin(), ys.end());
		float margin = 80.0f;
		float width = maxx - minx + margin*2;
		float height = maxy - miny + margin*2;
		float zx = (screenWidth) / width;
		float zy = (screenHeight) / height;
		zoom = std::min(std::max(std::min(zx, zy), 0.1f), 4.0f);
		pan.x = -(minx + maxx)/2 + screenWidth/(2*zoom);
		pan.y = -(miny + maxy)/2 + screenHeight/(2*zoom);
	};

	
	Font uiFont = GetFontDefault();
	bool uiFontLoaded = false;
	const char *fontPathCandidates[] = { "src/resources/JetBrainsMono-Regular.ttf", "resources/JetBrainsMono-Regular.ttf" };
	for (auto &p : fontPathCandidates) {
		if (FileExists(p)) {
			uiFont = LoadFontEx(p, 20, 0, 0);
			uiFontLoaded = true;
			break;
		}
	}

	while (!WindowShouldClose()) {
		
		if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
			dragging = true;
			lastMouse = GetMousePosition();
		}
		if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) dragging = false;
		if (dragging) {
			Vector2 m = GetMousePosition();
			pan.x += (m.x - lastMouse.x) / zoom;
			pan.y += (m.y - lastMouse.y) / zoom;
			lastMouse = m;
		}

		float wheel = GetMouseWheelMove();
		if (wheel != 0) {
			float oldZoom = zoom;
			zoom *= (1.0f + wheel * 0.1f);
			if (zoom < 0.1f) zoom = 0.1f;
			if (zoom > 4.0f) zoom = 4.0f;
			
			Vector2 m = GetMousePosition();
			pan.x = (pan.x - m.x / oldZoom) * (zoom / oldZoom) + m.x / zoom;
			pan.y = (pan.y - m.y / oldZoom) * (zoom / oldZoom) + m.y / zoom;
		}

		
		if (IsKeyPressed(KEY_A)) { 
			int val = nextRandom++;
			tree.insert(val);
			
			std::vector<float> xs, ys;
			int cursor = 0;
			tree.traverse([&](BTree::Node* node, int depth, int index){ xs.push_back(cursor*40+100); ys.push_back(depth*80+50); cursor+=2; });
			fitView(xs, ys);
		}
		if (IsKeyPressed(KEY_M)) { 
			typing = true; typed = ""; typingMode = TypingMode::Multi;
		}
		if (IsKeyPressed(KEY_I)) { 
			typing = true; typed = ""; typingMode = TypingMode::Insert;
		}
		if (IsKeyPressed(KEY_D)) { 
			tree.erase(nextRandom - 1);
			nextRandom = std::max(100, nextRandom - 1);
		}
		if (IsKeyPressed(KEY_X)) { 
			tree.clearAll();
			nextRandom = 100;
		}
		if (IsKeyPressed(KEY_H)) { 
			if (hoveredKey != -1) tree.erase(hoveredKey);
		}
		if (IsKeyPressed(KEY_Z)) { 
			std::vector<float> xs, ys;
			int cursor = 0;
			tree.traverse([&](BTree::Node* node, int depth, int index){ xs.push_back(cursor*40+100); ys.push_back(depth*80+50); cursor+=2; });
			fitView(xs, ys);
		}
		if (IsKeyPressed(KEY_R)) { 
			tree.clearAll(); nextRandom = 100;
			
			for (int i=0;i<8;i++) { tree.insert(10 + rand()%90); }
			
			std::vector<float> xs, ys; int cursor=0;
			tree.traverse([&](BTree::Node* node, int depth, int index){ xs.push_back(cursor*40+100); ys.push_back(depth*80+50); cursor+=2; });
			fitView(xs, ys);
		}

		
		int ch = GetCharPressed();
		while (ch > 0) {
			if (typing) {
				char c = (char)ch;
				if ((c >= '0' && c <= '9') || c=='-' ) typed.push_back(c);
			}
			ch = GetCharPressed();
		}
		if (typing) {
			if (IsKeyPressed(KEY_BACKSPACE) && !typed.empty()) typed.pop_back();
			if (IsKeyPressed(KEY_ESCAPE)) { typing = false; typed.clear(); typingMode = TypingMode::None; }
			if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER)) {
				if (!typed.empty()) {
					try {
						int v = std::stoi(typed);
						if (typingMode == TypingMode::Insert) {
							tree.insert(v);
						} else if (typingMode == TypingMode::Multi) {
							int count = std::max(0, v);
							for (int i=0;i<count;i++) { tree.insert(nextRandom++); }
						}
					} catch(...) {}
				}
				typing = false; typed.clear(); typingMode = TypingMode::None;
				
				std::vector<float> xs, ys; int cursor=0;
				tree.traverse([&](BTree::Node* node, int depth, int index){ xs.push_back(cursor*40+100); ys.push_back(depth*80+50); cursor+=2; });
				fitView(xs, ys);
			}
		}
		if (IsKeyPressed(KEY_KP_ADD) || IsKeyPressed(KEY_EQUAL)) zoom = std::min(zoom * 1.1f, 4.0f);
		if (IsKeyPressed(KEY_KP_SUBTRACT) || IsKeyPressed(KEY_MINUS)) zoom = std::max(zoom * 0.9f, 0.1f);

		
		BeginDrawing();
		ClearBackground(RAYWHITE);

	Camera2D camera;
	camera.offset = {0.0f, 0.0f};
	camera.target = {-pan.x, -pan.y};
	camera.rotation = 0.0f;
	camera.zoom = zoom;
	BeginMode2D(camera);

		int yStart = 50;
		int levelHeight = 80;
		int xSpacing = 40;

		struct DrawCtx { int yStart; int levelHeight; int xSpacing; Vector2 mouseWorld; int hoveredKey; } ctx;
		ctx.yStart = yStart; ctx.levelHeight = levelHeight; ctx.xSpacing = xSpacing;
		Vector2 mp = GetMousePosition();
		
		ctx.mouseWorld = GetScreenToWorld2D(mp, camera);
		ctx.hoveredKey = -1;

		
		struct KeyPos { BTree::Node* node; int depth; int idx; float x; float y; int value; };
		std::vector<KeyPos> keyPositions;
		int cursorX = 0;
		tree.traverse([&](BTree::Node* node, int depth, int index) {
			float x = cursorX * ctx.xSpacing + 100;
			float y = ctx.yStart + depth * ctx.levelHeight;
			int v = node->keys[index];
			keyPositions.push_back(KeyPos{node, depth, index, x, y, v});
			cursorX += 2;
		});

		
		std::unordered_map<BTree::Node*, std::vector<KeyPos>> nodeMap;
		std::vector<BTree::Node*> nodeOrder; nodeOrder.reserve(64);
		for (auto &kp : keyPositions) {
			if (nodeMap.find(kp.node) == nodeMap.end()) nodeOrder.push_back(kp.node);
			nodeMap[kp.node].push_back(kp);
		}

		
		struct NodeLayout { float minx, maxx, cx, cy; int depth; };
		std::unordered_map<BTree::Node*, NodeLayout> layouts;
		
		for (auto *n : nodeOrder) {
			auto &vec = nodeMap[n];
			float minx = FLT_MAX, maxx = -FLT_MAX; int depth = vec.empty() ? 0 : vec[0].depth;
			for (auto &kp : vec) { minx = std::min(minx, kp.x); maxx = std::max(maxx, kp.x); }
			if (minx==FLT_MAX) { minx = 0; maxx = 0; }
			float cx = (minx + maxx) * 0.5f;
			float cy = ctx.yStart + depth * ctx.levelHeight;
			layouts[n] = NodeLayout{minx, maxx, cx, cy, depth};
		}

		

		
		
		std::unordered_map<BTree::Node*, std::vector<float>> nodePointerXs;
		for (auto &kv : nodeMap) {
			BTree::Node* node = kv.first;
			auto vec = kv.second; 
			std::sort(vec.begin(), vec.end(), [](const KeyPos &a, const KeyPos &b){ return a.x < b.x; });
			std::vector<float> keyXs;
			for (auto &kp : vec) keyXs.push_back(kp.x);
			float leftEdge = keyXs.front() - 30.0f;
			float rightEdge = keyXs.back() + 30.0f;
			std::vector<float> ptrXs;
			ptrXs.reserve(keyXs.size() + 1);
			ptrXs.push_back(leftEdge);
			for (size_t i = 1; i < keyXs.size(); ++i) ptrXs.push_back((keyXs[i-1] + keyXs[i]) * 0.5f);
			ptrXs.push_back(rightEdge);
			nodePointerXs[node] = ptrXs;
		}

		
		for (auto &kv : nodeMap) {
			BTree::Node* node = kv.first;
			auto vec = kv.second;
			std::sort(vec.begin(), vec.end(), [](const KeyPos &a, const KeyPos &b){ return a.x < b.x; });
			auto L = layouts[node];
			auto &keyXs = nodePointerXs[node];
			
			float left = keyXs.front() - 18.0f;
			float right = keyXs.back() + 18.0f;
			float nodeH = 36.0f;
			Rectangle nodeRect = { left, L.cy - nodeH/2.0f, right - left, nodeH };
			DrawRectangleRec(nodeRect, Fade(LIGHTGRAY, 0.95f));
			DrawRectangleLinesEx(nodeRect, 2, DARKGRAY);

			
			for (float px : keyXs) {
				DrawLineEx({px, L.cy - nodeH/2.0f}, {px, L.cy + nodeH/2.0f}, 2, DARKGRAY);
			}

			
			int fontSize = 16;
			for (size_t i = 0; i < vec.size(); ++i) {
				float leftCell = keyXs[i];
				float rightCell = keyXs[i+1];
				float tx = (leftCell + rightCell) * 0.5f;
				std::stringstream ss; ss << vec[i].value;
				std::string s = ss.str();
				Vector2 textSize = MeasureTextEx(uiFont, s.c_str(), fontSize, 1);
				Vector2 pos = { tx - textSize.x/2.0f, L.cy - textSize.y/2.0f };
				DrawTextEx(uiFont, s.c_str(), pos, fontSize, 1, BLACK);
				Rectangle keyRect = { tx - 18, L.cy - 18, 36, 36 };
				if (CheckCollisionPointRec(ctx.mouseWorld, keyRect)) {
					DrawRectangleLinesEx(keyRect, 2, GOLD);
					ctx.hoveredKey = vec[i].value;
				}
			}
			
			auto &ptrs = nodePointerXs[node];
			float pointerH = 8.0f;
			for (float px : ptrs) {
				Rectangle pr = { px - 6, L.cy + nodeH/2.0f + 6, 12, pointerH };
				DrawRectangleRec(pr, Fade(DARKGRAY, 0.8f));
			}
		}

		
		for (auto &kv : nodeMap) {
			BTree::Node* node = kv.first;
			auto &ptrs = nodePointerXs[node];
			for (size_t i = 0; i < node->children.size(); ++i) {
				BTree::Node* child = node->children[i];
				if (nodePointerXs.find(child) == nodePointerXs.end()) continue;
				float fromX = (i < ptrs.size()) ? ptrs[i] : ptrs.back();
				auto &childPtrs = nodePointerXs[child];
				
				float bestX = childPtrs.front(); float bestD = fabs(bestX - fromX);
				for (float cx : childPtrs) { float d = fabs(cx - fromX); if (d < bestD) { bestD = d; bestX = cx; } }
				float nodeH = 36.0f;
				
				float parentPtrY = layouts[node].cy + nodeH/2.0f + 6 + (8.0f/2.0f);
				
				float childCenterY = layouts[child].cy;
				DrawLineEx({fromX, parentPtrY}, {bestX, childCenterY}, 2.0f, DARKGRAY);
			}
		}
	EndMode2D();
	
	hoveredKey = ctx.hoveredKey;

	
	int hudFontSize = 16;
	std::vector<std::string> legend = {
		"A: add random",
		"M: add multiple (type count + Enter)",
		"I: insert typed",
		"D: delete last",
		"H: delete hovered",
		"X: clear all",
		"Drag: pan",
		"Wheel or +/-: zoom",
		"R: reset sample",
	};
	
	float padding = 12.0f;
	float lineSpacing = 6.0f;
	float maxW = 0;
	for (auto &s : legend) maxW = std::max(maxW, MeasureTextEx(uiFont, s.c_str(), hudFontSize, 1).x);
	float boxW = maxW + padding*2 + 20; 
	float lineH = MeasureTextEx(uiFont, "Tg", hudFontSize, 1).y;
	float boxH = (lineH + lineSpacing) * legend.size() + padding*2 - lineSpacing;
	float bx = screenWidth - boxW - 16.0f;
	float by = screenHeight - boxH - 16.0f;

	
	Rectangle legendRect = { bx, by, boxW, boxH };
	DrawRectangleRounded(legendRect, 0.12f, 6, Fade(RAYWHITE, 0.95f));
	DrawRectangleRoundedLines(legendRect, 0.12f, 6, 2, DARKGRAY);

	
	for (size_t i = 0; i < legend.size(); ++i) {
		float tx = bx + padding + 18;
		float ty = by + padding + i * (lineH + lineSpacing);
		
		Rectangle icon = { bx + padding, ty + (lineH - 12)/2.0f, 12, 12 };
		DrawRectangleRec(icon, Fade(DARKGRAY, 0.9f));
		
		DrawTextEx(uiFont, legend[i].c_str(), {tx, ty}, hudFontSize, 1, DARKGRAY);
	}

	if (typing) {
		std::string t = "Typing: "+typed+" (Enter to commit, Esc to cancel)";
		DrawTextEx(uiFont, t.c_str(), {bx + padding, by - 24}, hudFontSize, 1, RED);
	}

		EndDrawing();
	}

	CloseWindow();
	return 0;
}

