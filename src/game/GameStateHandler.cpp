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

	sol::table rulesTable = lua["rules"];

	auto exists_in_rules = [&](const string& key) -> bool {
		if (!rulesTable.valid() || rulesTable.get_type() != sol::type::table)
			return false;
		const sol::object v = rulesTable[key];
		return v.valid() && v.get_type() == sol::type::table;
	};

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
    	sol::table enemies = lua.create_table();
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

    			sol::table e = lua.create_table();
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
    	string env;
    	if (parsed.contains("environment") && parsed["environment"].is_string()) {
    		env = parsed["environment"].get<string>();
    	} else {
    		env.clear();
    	}

    	gameState["environment"] = env;
	}
}
