#include "MusicMaker.h"

using namespace std;
using namespace sol;


void MusicMaker::startGameStateThread() {
	std::thread([this] {
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
				table gameState = lua.create_table();

				handleGameState(parsed, gameState);

				// Call on_update
				onUpdate(gameState);

			} catch (const exception& e) {
				cerr << "[MusicMaker] JSON parse error: " << e.what() << endl;
			}
		}
	}).detach();
}


void MusicMaker::handleGameState(nlohmann::json& parsed, table& gameState) {
    using nlohmann::json;

    auto get_number = [&](const json& j, const char* key, double& out)->bool {
		const auto it = j.find(key);
        if (it != j.end() && it->is_number()) {
	        out = it->get<double>();
        	return true;
        }
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
    	table enemies = lua.create_table();
    	size_t written = 0;

    	if (parsed.contains("enemies") && parsed["enemies"].is_array()) {
    		constexpr size_t kMaxEnemies = 256;
    		for (const auto& enemy : parsed["enemies"]) {
    			if (!enemy.is_object()) continue;

    			string type;
    			if (!enemy.contains("type") || !enemy["type"].is_string())
    				continue;
    			type = enemy["type"].get<string>();
    			if (type.empty()) continue;

    			table e = lua.create_table();
    			e["type"] = type;
    			if (enemy.contains("distance") && enemy["distance"].is_number()) {
    				double d = enemy["distance"].get<double>();
    				e["distance"] = d < 0.0 ? 0.0 : d;
    			} else {
    				e["distance"] = 0.0;
    			}

    			enemies[++written] = e;
    			if (written >= kMaxEnemies) {
    				cerr << "[MusicMaker] JSON: enemies truncated at " << kMaxEnemies << "\n";
    				break;
    			}
    		}
    	}
    	gameState["enemies"] = enemies;
	}

    // environment
	{
    	string envStr;
    	table tagsTbl = lua.create_table();

    	if (parsed.contains("environment")) {
    		const auto& env = parsed["environment"];

    		if (env.is_object()) {
    			// Prefer object shape: { type: string, tags: string[] }
    			if (env.contains("type") && env["type"].is_string()) {
    				envStr = env["type"].get<string>();
    			}
    			if (env.contains("tags") && env["tags"].is_array()) {
    				size_t i = 0;
    				for (const auto& t : env["tags"]) {
    					if (t.is_string()) {
    						tagsTbl[++i] = t.get<string>();
    					}
    				}
    			}
    		} else if (env.is_string()) {
    			// Old shape: environment was a single string
    			envStr = env.get<string>();
    		} else {
    			cerr << "[MusicMaker] JSON: environment wrong type -> default empty\n";
    		}
    	}

    	gameState["environment"]     = envStr;
    	gameState["environmentTags"] = tagsTbl;
	}
}
