#include <iostream>
#include <iomanip>
#include <chrono>
#include <thread>

class progressBar {
private:
    double progress;  // Declare the static member
    int barWidth; // Declare the static member
public:
    progressBar() {progress = 0.0; barWidth = 50; }

    void setBarWidht(int w){
        barWidth = w;
    }

    void getProgress(){return progress;}

    void updateProgressBar() {
        float progressRatio = static_cast<float>(progress) / 100.0f;
        int barProgress = static_cast<int>(barWidth * progressRatio);

        std::cout << "\r[";
        for (int i = 0; i < barWidth; ++i) {
            if (i < barProgress) {
                std::cout << "=";
            } else {
                std::cout << " ";
            }
        }

        std::cout << "] " << std::setw(3) << progress << "%";
        std::cout.flush();
    }

    void updatePercent(double _progress) {
        progress = _progress;
        updateProgressBar();
    }
};

// Define the static member outside the class
// double progressBar::progress = 0.0;

int main() {
    progressBar myProgressBar;

    for (int i = 0; i <= 100; ++i) {
        myProgressBar.updatePercent(i);

        // Simulate some work
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    std::cout << std::endl;

    return 0;
}
