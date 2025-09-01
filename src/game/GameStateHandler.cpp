#include "MusicMaker.h"

using namespace std;


void MusicMaker::startGameStateThread() {
	thread([this] {
		while (!stopReceiver.load() && gb.isClientConnected()) {
			string payload = gb.receiveJsonPayload();
			if (payload.empty()) {
				if (gb.isClientConnected()) {
					// Client still connected -> Game paused
					pause();
				}
				continue;
			}

			if (isPaused) {
				isPaused = false;
				resume();
			}

			try {
				nlohmann::json parsed = nlohmann::json::parse(payload);

				// Convert to Lua table
				sol::table gameState = lua.create_table();

				handleGameState(parsed, gameState);

				// Call on_update
				onUpdate(gameState);

			} catch (const exception& e) {
				cerr << "[MusicMaker] JSON parse error: " << e.what() << endl;
			}
		}
	}).detach();
}


void MusicMaker::handleGameState(nlohmann::json& parsed, sol::table& gameState) {
    using nlohmann::json;

    auto get_number = [&](const json& j, const char* key, double& out)->bool {
		const auto it = j.find(key);
        if (it != j.end() && it->is_number()) { out = it->get<double>(); return true; }
        return false;
    };

    auto get_string = [&](const json& j, const char* key, string& out)->bool {
        const auto it = j.find(key);
        if (it != j.end() && it->is_string()) { out = it->get<string>(); return true; }
        return false;
    };

    // playerHealth
    {
        double hp = 1.0;  // Default
        if (parsed.contains("playerHealth")) {
            if (!get_number(parsed, "playerHealth", hp)) {
                cerr << "[MusicMaker] JSON: playerHealth wrong type -> default 1.0\n";
                hp = 1.0;
            }
        }
        if (hp < 0.0 || hp > 1.0) {
            cerr << "[MusicMaker] JSON: playerHealth out of range -> clamped\n";
            hp = clamp(hp, 0.0, 1.0);
        }
        gameState["playerHealth"] = hp;
    }

    // enemies
    {
        sol::table enemies = lua.create_table();
        size_t written = 0;

        if (parsed.contains("enemies")) {
            const auto& arr = parsed["enemies"];
            if (!arr.is_array()) {
                cerr << "[MusicMaker] JSON: enemies not an array -> set empty\n";
            } else {
                constexpr size_t kMaxEnemies = 256;
                for (const auto& enemy : arr) {
                    if (!enemy.is_object()) continue;

                    string type;
                    if (!get_string(enemy, "type", type) || type.empty()) {
                        cerr << "[MusicMaker] JSON: enemy missing/empty 'type' -> skipped\n";
                        continue;
                    }

                    double dist = 0.0;
                    if (enemy.contains("distance")) {
                        if (enemy["distance"].is_number()) {
                            dist = enemy["distance"].get<double>();
                            if (dist < 0.0) { dist = 0.0; }
                        } else {
                            cerr << "[MusicMaker] JSON: enemy.distance wrong type -> ignored\n";
                        }
                    }

                    sol::table e = lua.create_table();
                    e["type"] = type;
                    e["distance"] = dist;
                    enemies[++written] = e;

                    if (written >= kMaxEnemies) {
                        cerr << "[MusicMaker] JSON: enemies truncated at " << kMaxEnemies << "\n";
                        break;
                    }
                }
            }
        }
        gameState["enemies"] = enemies;
    }

    // environment
    {
        string env;
        if (!get_string(parsed, "environment", env)) {
            cerr << "[MusicMaker] JSON: environment missing/wrong type -> set \"\"\n";
            env.clear();  // neutral
        }
        gameState["environment"] = env;
    }
}
