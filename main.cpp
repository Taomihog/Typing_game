/*
A typing game
*/

#include <iostream>
#include <iomanip>
#include <fstream>
//#include <sstream>
#include <conio.h> // For _kbhit and _getch on Windows
#include <thread>
#include <chrono>
#include <atomic>
#include <unordered_map>
#include <vector>
#include <string>
#include <cmath>

class random_int_generator {

    public:
    random_int_generator() = delete;
    random_int_generator(int range): range(range) {};
    ~random_int_generator() = default;
    int operator()() {
        return generate_next(prev,range);
    }

    private:
    static constexpr double r = 3.9999;
    static int generate_next (double & prev, const double range) {
        double curr = r * prev * (1.0 - prev);
        prev = curr;
        return static_cast<int>(curr * range);
    }
    double range = 100;
    double prev = 0.1;//seed
};

struct key_info {//struct to store your typing speed, error rate, etc for a character.
    key_info() = default;
    key_info(char ch): ch(ch) {};
    key_info(char ch, int occ, int error, int sum_t, int sum_tsquare): 
        ch(ch),
        occ(occ),
        error(error),
        sum_t(sum_t),
        sum_tsquare(sum_tsquare) {};
	char ch = ' ';
	int occ = 0;//occurance
	int error = 0;//#correct = #occ - #error
	int sum_t = 5000;//millisec, used to calculate the mean time to type this char
	int sum_tsquare = sum_t * sum_t;//used to calculate the var time to type this char
};

class typing_game {//intialize data, save

	public:
	typing_game() = default;

    typing_game(std::string keys): keys(keys), rnd(keys.size()) {
        for(const auto & ch : keys) {
            all_key_info.insert(std::pair<char, key_info>(ch, key_info(ch)));
        }
    }

	char generate() {return keys[rnd()];};//generate a random char using the data. The less familar with the char, the higher the odd to generate it.

	int suggested_time(char key) const {
        //std::cout << "suggested_time char=" << key << std::endl;
        const key_info & info = all_key_info.at(key);
        int mean = info.sum_t / (info.occ+1);
        int sd = std::sqrt((info.sum_tsquare - info.sum_t * info.sum_t / (info.occ + 1)) / (info.occ + 1));
        return mean;
        return mean + sd;//suggested time equals to mean + 1 sigma
    }

    void update_key_info(char key, bool correct, double time) {
        key_info & info = all_key_info.at(key);
        ++ info.occ;
        if (!correct) {
            ++ info.error;
        }
        info.sum_t += time;
        info.sum_tsquare += time * time;
    }

    void print_key_info() const {
        std::cout << "========================Score========================" <<std::endl;
        for(auto data : all_key_info) {
            std::cout << "key_to_type = " << data.first;
            std::cout << ", occ = " << data.second.occ;
            std::cout << ", error = " << data.second.error;
            std::cout << ", mean time = " << suggested_time(data.first) << "s.";
            std::cout << std::endl;
        }
    }

    const key_info & get_key_info(char key) const { return all_key_info.at(key); }

	bool try_load_key_info(std::string file_path) {
        std::ifstream inputFile(file_path);
        if (!inputFile.is_open()) {
            std::cerr << "Failed to open the file for reading." << std::endl;
            return false; // Return an error code if the file couldn't be opened
        }

        keys.clear();
        all_key_info.clear();

        std::string line;
        while (std::getline(inputFile, line)) {
            // Process the line (e.g., print it)
            std::vector<std::string> data_strings;
            size_t pos = line.find('\t');
            while(pos != std::string::npos) {
                data_strings.emplace_back(line.substr(0, pos));
                line = line.substr(pos + 1);
                pos = line.find('\t');
            }
            data_strings.emplace_back(line);
            //std::cout << data_strings[0] << '\t' << data_strings[1] << '\t' << data_strings[2] << '\t' << data_strings[3] << '\t' << data_strings[4] << std::endl;
            key_info info ( data_strings[0][0], 
                            std::stoi(data_strings[1]), 
                            std::stoi(data_strings[2]), 
                            std::stoi(data_strings[3]), 
                            std::stoi(data_strings[4]));
            all_key_info.insert(std::pair<char, key_info>(info.ch, info));
            keys += info.ch;
        }
        inputFile.close();
        // std::cout << '|';
        // for(auto key_val : all_key_info) {
        //     std::cout << key_val.first << '|';
        // }
        // std::cout << '|' << std::endl;
        return true;
    }

	void save_key_info(std::string file_path) const {
        std::ofstream outputFile(file_path, std::ios::out | std::ios::trunc);
        //gpt said: 
        //The std::ios::out mode specified when opening the file will clear the existing content, but it assumes the file already exists.
        //std::ios::trunc is used in conjunction with std::ios::out to create a new file or truncate (clear) the existing content if the file already exists.
        if (!outputFile.is_open()) {
            std::cerr << "Failed to open the file for writing." << std::endl;
            return; // Return an error code if the file couldn't be opened
        }
        
        for(auto data : all_key_info) {
            key_info kinfo = data.second;
            outputFile << kinfo.ch << "\t" << kinfo.occ << "\t" << kinfo.error << "\t" << kinfo.sum_t << "\t" << kinfo.sum_tsquare << std::endl;
        }

        outputFile.close();
    }
	
	private:
    std::string keys;
    std::unordered_map<char, key_info> all_key_info;
    random_int_generator rnd;
};

std::atomic<int> timer(5000); // Use an atomic double for the timer
std::atomic<bool> stop(false);
std::atomic<char> key_input (0);
std::atomic<bool> is_pressed(false);

const std::string default_keys ("`1234567890-=qwertyuiop[]\\asdfghjkl;'zxcvbnm,./~!@#$%^&*()_+QWERTYUIOP{}|ASDFGHJKL:\"ZXCVBNM<>?");
const std::vector<std::string> congrat {"Great!", "Bravo!!", "Correct~", "Excellent!", "Not bad;D", "Nice!", "**ImaginaryFireworks**"};
const std::vector<std::string> encourage {"Ganbatte!", "Incorrect..", "Try again", "O_o?", "Never give up!", "git gud~", "Keep fighting!!"};

void TimerThread() {
    typing_game tg(default_keys);
    tg.try_load_key_info("all_key_info.txt");
    char key_to_type(0);
    random_int_generator rnd2(congrat.size());

    while (timer > 0) {
        if(key_to_type == 0) {
            key_to_type = tg.generate();//generate a suggested char
            timer = tg.suggested_time(key_to_type);
            std::cout << "Type '" << key_to_type << "'";
            for(int i = 0; i < timer / 1000; ++i) {
                std::cout << '.';
            }// Display the timer in the console
        }//key_to_type and timer_start is all set, print-out is all done.

        //wait 100 ms and update timer
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        timer = timer - 100;
        if (timer % 1000 < 100) {
            std::cout << "\b \b";
        }

        //if is_pressed changed, update_key_info, then set key_to_type = 0;
		if (is_pressed) {//when time expired, this code block will not execute, therefore the key_info is not (and should not be) updated.

            is_pressed = false;
            tg.update_key_info(key_to_type, (key_to_type == key_input), timer);

            const key_info & kinfo = tg.get_key_info(key_to_type);
            std::cout << key_input << "\t" << (key_to_type == key_input ? congrat[rnd2()] : encourage[rnd2()]);
			std::cout << " (" << (kinfo.occ-kinfo.error) << "/" << kinfo.occ << ", " ;
            std::cout << std::fixed << std::setprecision(2) << 0.001*(tg.suggested_time(key_to_type) - timer) << " s. )" << std::endl;

            key_to_type = 0;
		}
    }
    stop = true;
    std::cout << std::endl << "Timer expired." << std::endl;
    //tg.print_key_info();
    tg.save_key_info("all_key_info.txt");
}

void KeyboardThread() {// a simple thread to listen to keyboard input
    while (true) {
        if (_kbhit()) { // Check if a key has been pressed
            key_input = _getch();    // Consume the keypress
			is_pressed = true;
        }
        if (stop) {
            return;
        }
    }
}

int main() {
    // Start TimerThread and KeyboardThread
    std::thread timerThread(TimerThread);
    std::thread keyboardThread(KeyboardThread);

    // Wait for TimerThread to finish
    timerThread.join();
    keyboardThread.join();

    return 0;
}