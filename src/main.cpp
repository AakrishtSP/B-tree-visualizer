
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
	const int screenWidth = 1200;
	const int screenHeight = 800;
	InitWindow(screenWidth, screenHeight, "B-Tree Visualizer (Animated)");
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

	
	// Load embedded font
	Font uiFont = LoadFontFromMemory(".ttf", embedded_font_data, embedded_font_data_size, 20, 0, 0);
	bool uiFontLoaded = true;

	while (!WindowShouldClose()) {
		float deltaTime = GetFrameTime();
		
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
				// Draw splitting animation - show keys redistributing
				DrawRectangleRec(nodeRect, Fade(YELLOW, 0.3f + 0.4f * sin(splitProgress * 3.14159f)));
				DrawRectangleLinesEx(nodeRect, 3, Fade(ORANGE, 0.8f));
				
				// Draw text to explain the split
				const char* splitText = "SPLITTING NODE...";
				Vector2 textSize = MeasureTextEx(uiFont, splitText, 14, 1);
				Vector2 textPos = { nodeRect.x + nodeRect.width/2 - textSize.x/2, nodeRect.y - 25 };
				DrawTextEx(uiFont, splitText, textPos, 14, 1, ORANGE);
			} else if (isViolation) {
				// Draw violation - node has too many keys
				float pulse = 0.5f + 0.5f * sin(GetTime() * 10.0f);
				DrawRectangleRec(nodeRect, Fade(violationColor, 0.3f * pulse));
				DrawRectangleLinesEx(nodeRect, 4, Fade(violationColor, 0.9f));
				
				// Draw text to explain the violation
				const char* violationText = "TOO MANY KEYS!";
				Vector2 textSize = MeasureTextEx(uiFont, violationText, 14, 1);
				Vector2 textPos = { nodeRect.x + nodeRect.width/2 - textSize.x/2, nodeRect.y - 25 };
				DrawTextEx(uiFont, violationText, textPos, 14, 1, violationColor);
			} else {
				DrawRectangleRec(nodeRect, Fade(LIGHTGRAY, 0.95f));
				DrawRectangleLinesEx(nodeRect, 2, DARKGRAY);
			}

			
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
					// Draw fading out key (shrinking and fading)
					float alpha = 1.0f - fadeProgress;
					float scale = 1.0f - fadeProgress * 0.5f;
					int fadeFontSize = (int)(fontSize * scale);
					DrawTextEx(uiFont, s.c_str(), pos, fadeFontSize, 1, Fade(RED, alpha));
					Rectangle keyRect = { tx - 18 * scale, L.cy - 18 * scale, 36 * scale, 36 * scale };
					DrawRectangleLinesEx(keyRect, 3, Fade(RED, alpha));
				} else if (isHighlighted) {
					DrawTextEx(uiFont, s.c_str(), pos, fontSize, 1, highlightColor);
					Rectangle keyRect = { tx - 18, L.cy - 18, 36, 36 };
					DrawRectangleLinesEx(keyRect, 3, highlightColor);
				} else {
					DrawTextEx(uiFont, s.c_str(), pos, fontSize, 1, BLACK);
				}
				
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
		
		// Draw animated keys moving
		for (const auto& anim : tree.getCurrentAnimations()) {
			if (anim.type == BTree::AnimationType::KeyMoving) {
				float t = easeInOutCubic(anim.progress);
				
				// Calculate actual target position based on endPos (already set to correct world coords)
				Vector2 targetPos = anim.endPos;
				
				// Convert screen start position to world coordinates
				Vector2 startWorld = GetScreenToWorld2D(anim.startPos, camera);
				
				// Interpolate position
				Vector2 currentPos;
				currentPos.x = startWorld.x + (targetPos.x - startWorld.x) * t;
				currentPos.y = startWorld.y + (targetPos.y - startWorld.y) * t;
				
				// Draw the moving key
				float scale = 1.0f + 0.3f * sin(anim.progress * 3.14159f);
				int fontSize = (int)(20 * scale);
				std::stringstream ss; ss << anim.movingKey;
				std::string s = ss.str();
				Vector2 textSize = MeasureTextEx(uiFont, s.c_str(), fontSize, 1);
				
				// Draw a glowing circle behind the key
				float radius = 24.0f * scale;
				DrawCircleV(currentPos, radius + 4, Fade(SKYBLUE, 0.3f));
				DrawCircleV(currentPos, radius, Fade(GOLD, 0.9f));
				DrawCircleLinesV(currentPos, radius, ORANGE);
				
				// Draw the key value
				Vector2 textPos = { currentPos.x - textSize.x/2.0f, currentPos.y - textSize.y/2.0f };
				DrawTextEx(uiFont, s.c_str(), textPos, fontSize, 1, BLACK);
				
				// Draw trail effect
				for (int i = 1; i <= 3; ++i) {
					float trailT = std::max(0.0f, t - i * 0.1f);
					Vector2 trailPos;
					trailPos.x = startWorld.x + (targetPos.x - startWorld.x) * trailT;
					trailPos.y = startWorld.y + (targetPos.y - startWorld.y) * trailT;
					float trailAlpha = 0.3f * (1.0f - i * 0.3f);
					DrawCircleV(trailPos, radius * 0.6f, Fade(GOLD, trailAlpha));
				}
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
	
	// Show animation status
	if (tree.isAnimating()) {
		std::string animText = "Animating...";
		Vector2 animTextSize = MeasureTextEx(uiFont, animText.c_str(), hudFontSize, 1);
		float animX = 20.0f;
		float animY = 20.0f;
		Rectangle animBox = {animX - 8, animY - 4, animTextSize.x + 16, animTextSize.y + 8};
		DrawRectangleRounded(animBox, 0.2f, 6, Fade(ORANGE, 0.8f));
		DrawTextEx(uiFont, animText.c_str(), {animX, animY}, hudFontSize, 1, WHITE);
	}

		EndDrawing();
	}

	CloseWindow();
	return 0;
}

