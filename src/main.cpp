
#include <raylib.h>
#include <string>
#include <sstream>
#include <cmath>
#include <algorithm>
#include <unordered_map>
#include <cfloat>
#include <random>
#include "btree.hpp"
#include "embedded_font.h"

// Helper function to ease animations
float easeInOutCubic(float t) {
    return t < 0.5f ? 4.0f * t * t * t : 1.0f - pow(-2.0f * t + 2.0f, 3.0f) / 2.0f;
}

int main() {
	int screenWidth = 1400;
	int screenHeight = 900;
	SetConfigFlags(FLAG_WINDOW_RESIZABLE);
	SetTraceLogLevel(LOG_NONE); // Disable all raylib logs
	InitWindow(screenWidth, screenHeight, "B-Tree Visualizer");
	SetTargetFPS(60);

	// Initialize random number generator with a proper seed
	std::random_device rd;
	std::mt19937 rng(rd());
	std::uniform_int_distribution<int> dist(10, 99);

	BTree tree(3);
	
	// Insert 8 unique random keys
	for (int i = 0; i < 8; ++i) {
		int val = dist(rng);
		while (tree.contains(val)) {
			val = dist(rng);
		}
		tree.insert(val);
	}

	Vector2 pan = {0, 0};
	float zoom = 1.0f;
	bool dragging = false;
	Vector2 lastMouse = {0, 0};

	int nextRandom = 100;
	int hoveredKey = -1;
	bool shouldFitViewAfterAnimation = false;
	
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

	
	// Load embedded fonts with higher resolution for better quality
	Font titleFont = LoadFontFromMemory(".ttf", embedded_font_data, embedded_font_data_size, 56, 0, 0);
	SetTextureFilter(titleFont.texture, TEXTURE_FILTER_BILINEAR);
	Font uiFont = LoadFontFromMemory(".ttf", embedded_font_data, embedded_font_data_size, 36, 0, 0);
	SetTextureFilter(uiFont.texture, TEXTURE_FILTER_BILINEAR);
	Font keyFont = LoadFontFromMemory(".ttf", embedded_font_data, embedded_font_data_size, 40, 0, 0);
	SetTextureFilter(keyFont.texture, TEXTURE_FILTER_BILINEAR);
	bool uiFontLoaded = true;

	// Fit to screen at start
	{
		std::vector<float> xs, ys;
		int cursor = 0;
		int yStart = 50, levelHeight = 80, xSpacing = 40;
		tree.traverse([&](BTree::Node* node, int depth, int index){ 
			xs.push_back(cursor * xSpacing + 100); 
			ys.push_back(yStart + depth * levelHeight); 
			cursor += 2; 
		});
		fitView(xs, ys);
	}

	while (!WindowShouldClose()) {
		float deltaTime = GetFrameTime();
		
		// Update screen dimensions if window was resized
		screenWidth = GetScreenWidth();
		screenHeight = GetScreenHeight();
		
		// Check if animations were running before update
		bool wasAnimating = tree.isAnimating();
		
		// Update animations
		tree.updateAnimation(deltaTime);
		
		// If animations just finished and we should fit view
		if (wasAnimating && !tree.isAnimating() && shouldFitViewAfterAnimation) {
			shouldFitViewAfterAnimation = false;
			std::vector<float> xs, ys;
			int cursor = 0;
			int yStart = 50, levelHeight = 80, xSpacing = 40;
			tree.traverse([&](BTree::Node* node, int depth, int index){ 
				xs.push_back(cursor * xSpacing + 100); 
				ys.push_back(yStart + depth * levelHeight); 
				cursor += 2; 
			});
			fitView(xs, ys);
		}
		
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

		
		// Input handling - only allow when not animating
		bool canInput = !tree.isAnimating();
		
		if (canInput && IsKeyPressed(KEY_A)) { 
			int val = dist(rng);
			// Ensure unique values
			while (tree.contains(val)) {
				val = dist(rng);
			}
			tree.insertAnimated(val);
			shouldFitViewAfterAnimation = true;
		}
		if (canInput && IsKeyPressed(KEY_M)) { 
			typing = true; typed = ""; typingMode = TypingMode::Multi;
		}
		if (canInput && IsKeyPressed(KEY_I)) { 
			typing = true; typed = ""; typingMode = TypingMode::Insert;
		}
		if (canInput && IsKeyPressed(KEY_D)) {
			// Delete last added key
			if (tree.hasKeys()) {
				int lastKey = tree.getLastInsertedKey();
				tree.eraseAnimated(lastKey);
			}
		}
		if (canInput && IsKeyPressed(KEY_X)) { 
			tree.clearAll();
			nextRandom = 100;
		}
		if (canInput && IsKeyPressed(KEY_H)) { 
			if (hoveredKey != -1) {
				tree.eraseAnimated(hoveredKey);
			}
		}
		if (canInput && IsKeyPressed(KEY_Z)) { 
			std::vector<float> xs, ys;
			int cursor = 0;
			int yStart = 50, levelHeight = 80, xSpacing = 40;
			tree.traverse([&](BTree::Node* node, int depth, int index){ 
				xs.push_back(cursor * xSpacing + 100); 
				ys.push_back(yStart + depth * levelHeight); 
				cursor += 2; 
			});
			fitView(xs, ys);
		}
		if (canInput && IsKeyPressed(KEY_R)) { 
			tree.clearAll(); nextRandom = 100;
			
			// Insert 8 unique random keys
			for (int i = 0; i < 8; i++) { 
				int val = dist(rng);
				while (tree.contains(val)) {
					val = dist(rng);
				}
				tree.insert(val);
			}
			
			// Fit view immediately for reset (no animation)
			std::vector<float> xs, ys; 
			int cursor = 0;
			int yStart = 50, levelHeight = 80, xSpacing = 40;
			tree.traverse([&](BTree::Node* node, int depth, int index){ 
				xs.push_back(cursor * xSpacing + 100); 
				ys.push_back(yStart + depth * levelHeight); 
				cursor += 2; 
			});
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
							// Check for duplicates before inserting
							if (!tree.contains(v)) {
								tree.insertAnimated(v);
							}
						} else if (typingMode == TypingMode::Multi) {
							int count = std::max(0, v);
							for (int i = 0; i < count; i++) { 
								int val = dist(rng);
								while (tree.contains(val)) {
									val = dist(rng);
								}
								tree.insertAnimated(val);
							}
						}
					} catch(...) {}
				}
				typing = false; typed.clear(); typingMode = TypingMode::None;
				shouldFitViewAfterAnimation = true;
			}
		}
		if (IsKeyPressed(KEY_KP_ADD) || IsKeyPressed(KEY_EQUAL)) zoom = std::min(zoom * 1.1f, 4.0f);
		if (IsKeyPressed(KEY_KP_SUBTRACT) || IsKeyPressed(KEY_MINUS)) zoom = std::max(zoom * 0.9f, 0.1f);

		
	BeginDrawing();
	// Modern gradient background
	ClearBackground(Color{245, 247, 250, 255});
	DrawRectangleGradientV(0, 0, screenWidth, screenHeight/3, 
		Color{240, 242, 245, 255}, Color{245, 247, 250, 255});	Camera2D camera;
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
			
			// Check if this node is being split or highlighted for violation
			bool isSplitting = false;
			bool isViolation = false;
			Color violationColor = RED;
			float splitProgress = 0.0f;
			
			for (const auto& anim : tree.getCurrentAnimations()) {
				if (anim.type == BTree::AnimationType::NodeSplitting && anim.operationNode == node) {
					isSplitting = true;
					splitProgress = easeInOutCubic(anim.progress);
					break;
				}
				if (anim.type == BTree::AnimationType::KeyHighlight && anim.highlightNode == node && anim.highlightKeyIndex == -1) {
					isViolation = true;
					violationColor = anim.highlightColor;
					break;
				}
			}
			
			if (isSplitting) {
				// Draw splitting animation with modern styling
				Color splitBg = Color{255, 200, 100, 255};
				Color splitBorder = Color{255, 140, 0, 255};
				
				// Rounded rectangle with shadow
				DrawRectangleRounded(Rectangle{nodeRect.x + 3, nodeRect.y + 3, nodeRect.width, nodeRect.height}, 
					0.25f, 8, Fade(BLACK, 0.15f));
				DrawRectangleRounded(nodeRect, 0.25f, 8, Fade(splitBg, 0.3f + 0.4f * sin(splitProgress * 3.14159f)));
				DrawRectangleRoundedLines(nodeRect, 0.25f, 8, 3, Fade(splitBorder, 0.9f));
				
				// Draw text to explain the split with background badge
				const char* splitText = "SPLITTING NODE...";
				Vector2 textSize = MeasureTextEx(uiFont, splitText, 13, 1);
				Vector2 textPos = { nodeRect.x + nodeRect.width/2 - textSize.x/2, nodeRect.y - 30 };
				Rectangle badgeRect = {textPos.x - 8, textPos.y - 4, textSize.x + 16, textSize.y + 8};
				DrawRectangleRounded(badgeRect, 0.3f, 6, splitBorder);
				DrawTextEx(uiFont, splitText, textPos, 13, 1, WHITE);
			} else if (isViolation) {
				// Draw violation with modern styling
				float pulse = 0.5f + 0.5f * sin(GetTime() * 10.0f);
				Color violationBg = Color{255, 80, 80, 255};
				
				// Rounded rectangle with shadow
				DrawRectangleRounded(Rectangle{nodeRect.x + 3, nodeRect.y + 3, nodeRect.width, nodeRect.height}, 
					0.25f, 8, Fade(BLACK, 0.15f));
				DrawRectangleRounded(nodeRect, 0.25f, 8, Fade(violationBg, 0.2f * pulse));
				DrawRectangleRoundedLines(nodeRect, 0.25f, 8, 4, Fade(violationColor, 0.9f));
				
				// Draw text to explain the violation with background badge
				const char* violationText = "TOO MANY KEYS!";
				Vector2 textSize = MeasureTextEx(uiFont, violationText, 13, 1);
				Vector2 textPos = { nodeRect.x + nodeRect.width/2 - textSize.x/2, nodeRect.y - 30 };
				Rectangle badgeRect = {textPos.x - 8, textPos.y - 4, textSize.x + 16, textSize.y + 8};
				DrawRectangleRounded(badgeRect, 0.3f, 6, violationColor);
				DrawTextEx(uiFont, violationText, textPos, 13, 1, WHITE);
			} else {
				// Normal node with modern styling - rounded corners and shadow
				Color nodeBg = Color{255, 255, 255, 255};
				Color nodeBorder = Color{100, 120, 150, 255};
				
				// Shadow
				DrawRectangleRounded(Rectangle{nodeRect.x + 2, nodeRect.y + 2, nodeRect.width, nodeRect.height}, 
					0.25f, 8, Fade(BLACK, 0.12f));
				// Node background
				DrawRectangleRounded(nodeRect, 0.25f, 8, nodeBg);
				// Border
				DrawRectangleRoundedLines(nodeRect, 0.25f, 8, 2.5f, nodeBorder);
			}

			
		
		// Draw cell dividers with modern subtle style
		for (float px : keyXs) {
			DrawLineEx({px, L.cy - nodeH/2.0f + 4}, {px, L.cy + nodeH/2.0f - 4}, 1.5f, 
				Fade(Color{180, 190, 200, 255}, 0.5f));
		}			
			int fontSize = 20;
			for (size_t i = 0; i < vec.size(); ++i) {
				float leftCell = keyXs[i];
				float rightCell = keyXs[i+1];
				float tx = (leftCell + rightCell) * 0.5f;
				std::stringstream ss; ss << vec[i].value;
				std::string s = ss.str();
				Vector2 textSize = MeasureTextEx(keyFont, s.c_str(), fontSize, 1);
				Vector2 pos = { tx - textSize.x/2.0f, L.cy - textSize.y/2.0f };
				
				// Store key position for animation system
				tree.setKeyPosition(node, i, {tx, L.cy});
				
				// Check if this key is being deleted (fading out)
				bool isFadingOut = false;
				float fadeProgress = 0.0f;
				
				// Check if this key is being highlighted or deleted
				bool isHighlighted = false;
				Color highlightColor = RED;
				for (const auto& anim : tree.getCurrentAnimations()) {
					if (anim.type == BTree::AnimationType::KeyHighlight && 
					    anim.highlightNode == node && anim.highlightKeyIndex == (int)i) {
						isHighlighted = true;
						highlightColor = anim.highlightColor;
						break;
					}
					// Check for deletion fade animation
					if (anim.type == BTree::AnimationType::KeyMoving && 
					    anim.targetNode == node && anim.targetIndex == (int)i && 
					    anim.operation == BTree::AnimationStep::None) {
						isFadingOut = true;
						fadeProgress = anim.progress;
						break;
					}
				}
				
			if (isFadingOut) {
				// Draw fading out key with modern effect
				float alpha = 1.0f - fadeProgress;
				float scale = 1.0f - fadeProgress * 0.5f;
				int fadeFontSize = (int)(fontSize * scale);
				Color fadeColor = Color{255, 80, 80, (unsigned char)(255 * alpha)};
					DrawTextEx(keyFont, s.c_str(), pos, fadeFontSize, 1, fadeColor);
					
					float circleRadius = 22.0f * scale;
					DrawCircleV({tx, L.cy}, circleRadius + 2, Fade(fadeColor, alpha * 0.3f));
					DrawCircleLinesV({tx, L.cy}, circleRadius, Fade(fadeColor, alpha * 0.9f));
				} else if (isHighlighted) {
					// Highlighted key with glow effect
					Color glowColor = highlightColor;
					DrawCircleV({tx, L.cy}, 26, Fade(glowColor, 0.2f));
					DrawCircleV({tx, L.cy}, 22, Fade(glowColor, 0.4f));
					DrawTextEx(keyFont, s.c_str(), pos, fontSize, 1, glowColor);
					DrawCircleLinesV({tx, L.cy}, 22, glowColor);
				} else {
					// Normal key with better styling
					DrawTextEx(keyFont, s.c_str(), pos, fontSize, 1, Color{40, 50, 65, 255});
				}
				
				// Hover effect with modern circle
				Rectangle keyRect = { tx - 22, L.cy - 22, 44, 44 };
				if (CheckCollisionPointRec(ctx.mouseWorld, keyRect)) {
					Color hoverColor = Color{255, 180, 0, 255};
					DrawCircleV({tx, L.cy}, 24, Fade(hoverColor, 0.15f));
					DrawCircleLinesV({tx, L.cy}, 24, hoverColor);
					ctx.hoveredKey = vec[i].value;
				}
			}
			
			// Draw child pointers with modern styling
			auto &ptrs = nodePointerXs[node];
			float pointerH = 8.0f;
			for (float px : ptrs) {
				float py = L.cy + 18.0f;
				DrawCircle(px, py, 4, Color{100, 120, 150, 255});
				DrawCircle(px, py, 2, Color{180, 190, 200, 255});
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
		
		// Draw animated keys moving with enhanced visuals
		for (const auto& anim : tree.getCurrentAnimations()) {
			if (anim.type == BTree::AnimationType::KeyMoving) {
				float t = easeInOutCubic(anim.progress);
				
				// Check if this is a deletion animation
				bool isDeletion = (anim.operation == BTree::AnimationStep::DeleteKey);
				
				Vector2 startWorld, targetPos, currentPos;
				
				if (isDeletion) {
					// For deletion: get current position from node and move UP and fade out
					if (anim.targetNode && tree.nodeKeyPositions.find(anim.targetNode) != tree.nodeKeyPositions.end() &&
					    anim.targetIndex < (int)tree.nodeKeyPositions[anim.targetNode].size()) {
						startWorld = tree.nodeKeyPositions[anim.targetNode][anim.targetIndex];
						// Move key upward and slightly to the side
						targetPos = {startWorld.x + 50.0f, startWorld.y - 200.0f};
					} else {
						continue; // Skip if we can't find the position
					}
					currentPos.x = startWorld.x + (targetPos.x - startWorld.x) * t;
					currentPos.y = startWorld.y + (targetPos.y - startWorld.y) * t;
				} else {
					// For insertion: normal behavior
					targetPos = anim.endPos;
					startWorld = GetScreenToWorld2D(anim.startPos, camera);
					currentPos.x = startWorld.x + (targetPos.x - startWorld.x) * t;
					currentPos.y = startWorld.y + (targetPos.y - startWorld.y) * t;
				}
				
				// Draw the moving key with modern styling
				float alpha = isDeletion ? (1.0f - t) : 1.0f; // Fade out for deletion
				float scale = isDeletion ? (1.0f - t * 0.3f) : (1.0f + 0.2f * sin(anim.progress * 3.14159f));
				int fontSize = (int)(24 * scale);
				std::stringstream ss; ss << anim.movingKey;
				std::string s = ss.str();
				Vector2 textSize = MeasureTextEx(keyFont, s.c_str(), fontSize, 1);
				
			// Draw glowing effect with multiple circles
			float radius = 28.0f * scale;
			Color glowColor1 = isDeletion ? Color{255, 80, 80, 255} : Color{100, 180, 255, 255};
			Color glowColor2 = isDeletion ? Color{255, 120, 120, 255} : Color{255, 200, 80, 255};				DrawCircleV(currentPos, radius + 12, Fade(glowColor1, 0.15f * alpha));
				DrawCircleV(currentPos, radius + 6, Fade(glowColor1, 0.25f * alpha));
				DrawCircleV(currentPos, radius + 3, Fade(glowColor2, 0.4f * alpha));
				DrawCircleV(currentPos, radius, Fade(WHITE, alpha));
				DrawCircleLinesV(currentPos, radius, Fade(glowColor2, alpha));
				DrawCircleLinesV(currentPos, radius - 2, Fade(glowColor2, 0.5f * alpha));
				
			
			// Draw the key value
			Vector2 textPos = { currentPos.x - textSize.x/2.0f, currentPos.y - textSize.y/2.0f };
			DrawTextEx(keyFont, s.c_str(), textPos, fontSize, 1, Fade(Color{40, 50, 65, 255}, alpha));				// Draw enhanced trail effect (only for insertion)
				if (!isDeletion) {
					for (int i = 1; i <= 5; ++i) {
						float trailT = std::max(0.0f, t - i * 0.08f);
						Vector2 trailPos;
						trailPos.x = startWorld.x + (targetPos.x - startWorld.x) * trailT;
						trailPos.y = startWorld.y + (targetPos.y - startWorld.y) * trailT;
						float trailAlpha = 0.4f * (1.0f - i * 0.18f);
						float trailScale = 1.0f - i * 0.12f;
						DrawCircleV(trailPos, radius * trailScale * 0.7f, Fade(glowColor2, trailAlpha));
					}
				}
			}
		}
		
	EndMode2D();
	
	hoveredKey = ctx.hoveredKey;

// Modern title bar
Rectangle titleBar = {0, 0, (float)screenWidth, 60};
DrawRectangleGradientV(0, 0, screenWidth, 60, 
	Color{55, 65, 81, 255}, Color{75, 85, 99, 255});	const char* title = "B-Tree Visualizer";
	Vector2 titleSize = MeasureTextEx(titleFont, title, 28, 1);
	DrawTextEx(titleFont, title, {20, 16}, 28, 1, WHITE);
	
	// Subtitle
	const char* subtitle = "Interactive Animation & Exploration";
	DrawTextEx(uiFont, subtitle, {22, 44}, 14, 1, Fade(WHITE, 0.7f));

	
	int hudFontSize = 16;
	std::vector<std::string> legend = {
		"A  Add random key",
		"M  Add multiple keys",
		"I  Insert typed value",
		"D  Delete last added",
		"H  Delete hovered key",
		"X  Clear all keys",
		"Z  Zoom to fit",
		"R  Reset with samples",
		"",
		"Drag  Pan view",
		"Wheel  Zoom",
	};
	
	float padding = 16.0f;
	float lineSpacing = 8.0f;
	float maxW = 0;
	for (auto &s : legend) {
		if (!s.empty()) {
			maxW = std::max(maxW, MeasureTextEx(uiFont, s.c_str(), hudFontSize, 1).x);
		}
	}
	float boxW = maxW + padding*2 + 40; // Extra padding to prevent overflow
	float lineH = MeasureTextEx(uiFont, "Tg", hudFontSize, 1).y;
	// Count non-empty lines for proper height calculation
	int nonEmptyLines = 0;
	for (auto &s : legend) {
		if (!s.empty()) nonEmptyLines++;
	}
	float boxH = (lineH + lineSpacing) * nonEmptyLines + padding*2 + 36; // +36 for title and separator
	float bx = screenWidth - boxW - 20.0f;
	float by = 80.0f;

	

Rectangle legendRect = { bx, by, boxW, boxH };
// Shadow
DrawRectangleRounded(Rectangle{bx + 3, by + 3, boxW, boxH}, 0.15f, 8, Fade(BLACK, 0.2f));
// Background
DrawRectangleRounded(legendRect, 0.15f, 8, Fade(Color{255, 255, 255, 255}, 0.96f));
DrawRectangleRoundedLines(legendRect, 0.15f, 8, 2, Color{200, 210, 220, 255});	// Legend title
	const char* legendTitle = "Controls";
	Vector2 legendTitleSize = MeasureTextEx(uiFont, legendTitle, 18, 1);
	DrawTextEx(uiFont, legendTitle, {bx + padding, by + padding - 2}, 18, 1, Color{55, 65, 81, 255});
	
	// Separator line
	DrawLineEx({bx + padding, by + padding + 22}, 
		{bx + boxW - padding, by + padding + 22}, 2, Fade(Color{200, 210, 220, 255}, 0.5f));

	
	for (size_t i = 0; i < legend.size(); ++i) {
		if (legend[i].empty()) continue; // Skip empty lines
		
		float tx = bx + padding + 28;
		float ty = by + padding + 32 + i * (lineH + lineSpacing);
		
		// Check if this is a keyboard shortcut (starts with single letter)
		bool isKeyShortcut = legend[i].length() > 2 && legend[i][1] == ' ';
		
		if (isKeyShortcut) {
			// Draw key icon
			char keyChar[2] = {legend[i][0], '\0'};
			Rectangle keyIcon = { bx + padding, ty - 2, 20, 20 };
			DrawRectangleRounded(keyIcon, 0.25f, 4, Color{100, 180, 255, 255});
			DrawRectangleRoundedLines(keyIcon, 0.25f, 4, 1.5f, Color{70, 140, 220, 255});
			
			Vector2 keyTextSize = MeasureTextEx(uiFont, keyChar, 14, 1);
			DrawTextEx(uiFont, keyChar, 
				{keyIcon.x + 10 - keyTextSize.x/2, keyIcon.y + 10 - keyTextSize.y/2}, 
				14, 1, WHITE);
			
			// Draw description
			const char* desc = legend[i].c_str() + 2;
			DrawTextEx(uiFont, desc, {tx, ty}, hudFontSize, 1, Color{75, 85, 99, 255});
		} else {
			// Draw mouse action icon
			DrawCircle(bx + padding + 8, ty + 8, 6, Color{150, 160, 170, 255});
			DrawCircle(bx + padding + 8, ty + 8, 4, Color{200, 210, 220, 255});
			
			DrawTextEx(uiFont, legend[i].c_str(), {tx, ty}, hudFontSize, 1, Color{75, 85, 99, 255});
		}
	}

	if (typing) {
		std::string promptText = typingMode == TypingMode::Multi ? 
			"Enter number of keys to add: " : "Enter value to insert: ";
		std::string fullText = promptText + typed + "_";
		
		// Modern input box
		Vector2 textSize = MeasureTextEx(uiFont, fullText.c_str(), 18, 1);
		float inputBoxW = std::max(300.0f, textSize.x + 40);
		float inputBoxH = 60;
		float inputX = (screenWidth - inputBoxW) / 2;
		float inputY = screenHeight - 100;
		
		Rectangle inputBox = {inputX, inputY, inputBoxW, inputBoxH};
		// Shadow
		DrawRectangleRounded(Rectangle{inputX + 3, inputY + 3, inputBoxW, inputBoxH}, 
			0.2f, 8, Fade(BLACK, 0.25f));
		// Background
		DrawRectangleRounded(inputBox, 0.2f, 8, Color{255, 255, 255, 255});
		DrawRectangleRoundedLines(inputBox, 0.2f, 8, 3, Color{100, 180, 255, 255});
		
		DrawTextEx(uiFont, fullText.c_str(), 
			{inputX + 20, inputY + (inputBoxH - textSize.y)/2}, 18, 1, Color{40, 50, 65, 255});
		
		// Help text
		const char* helpText = "Enter to submit • Esc to cancel";
		Vector2 helpSize = MeasureTextEx(uiFont, helpText, 13, 1);
		DrawTextEx(uiFont, helpText, 
			{inputX + (inputBoxW - helpSize.x)/2, inputY - 20}, 13, 1, Color{120, 130, 140, 255});
	}
	
	// Show animation status with modern badge
	if (tree.isAnimating()) {
		std::string animText = "Animating...";
		Vector2 animTextSize = MeasureTextEx(uiFont, animText.c_str(), 16, 1);
		float animX = 20.0f;
		float animY = 80.0f;
		Rectangle animBox = {animX, animY, animTextSize.x + 24, animTextSize.y + 16};
		
		// Pulsing effect
		float pulse = 0.8f + 0.2f * sin(GetTime() * 4.0f);
		
		// Shadow
		DrawRectangleRounded(Rectangle{animX + 2, animY + 2, animBox.width, animBox.height}, 
			0.3f, 8, Fade(BLACK, 0.2f));
		// Background with pulse
		DrawRectangleRounded(animBox, 0.3f, 8, Fade(Color{255, 160, 50, 255}, pulse));
		DrawRectangleRoundedLines(animBox, 0.3f, 8, 2, Color{255, 140, 0, 255});
		
		// Animated dots
		int dotCount = ((int)(GetTime() * 3) % 4);
		std::string dotsText = animText.substr(0, 10);
		for (int i = 0; i < dotCount; i++) dotsText += ".";
		
		DrawTextEx(uiFont, dotsText.c_str(), {animX + 12, animY + 8}, 16, 1, WHITE);
	}

		EndDrawing();
	}

	CloseWindow();
	return 0;
}

