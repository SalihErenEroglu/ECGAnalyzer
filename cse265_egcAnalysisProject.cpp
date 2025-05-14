#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <stdexcept>

using namespace std;

class ECGProcessor { //Hilal Baþak
private:

    const double MIN_RR_INTERVAL = 0.3;

    const double INTERVAL_SIZE = 5.0; // second
    const double STEP_SIZE = 5.0; // second       

    struct ECGRecord {
        vector<double> time;
        vector<double> voltage;
    };

    struct Beat {
        double rPeakTime;
    };

    ECGRecord readECGFile(const string& filename) const { //Ceyda, Salih Eren
        ECGRecord record;
        ifstream infile(filename);
        if (!infile.is_open()) {
            throw runtime_error("File can not be opened: " + filename);
        }

        string line;
        while (getline(infile, line)) {
            if (line.empty()) {
                continue;
            }

            istringstream ss(line);
            double timeVal, voltageVal;
            if (!(ss >> timeVal >> voltageVal)) {               
                cerr << "Error: Incorrectly formatted line in file " << line << endl;
                continue;
            }

            record.time.push_back(timeVal);
            record.voltage.push_back(voltageVal);
        }

        infile.close();
        return record;
    }

	string classifyBeat(double rrInterval) const { //Nurettin Arda, Salih Eren
        
        double heartRate = 60.0 / rrInterval;
        if (heartRate < 60.0) {
            return "Bradycardia";
        }
        else if (heartRate > 100.0) {
            return "Tachycardia";
        }
        else {
            return "Normal";
        }
    }

    double findThreshold(const ECGRecord& record, double start_time, double end_time) const { //Nurettin Arda
        const auto& time = record.time;
        const auto& voltage = record.voltage;

        double maxVoltage = 0.0;
        for (size_t i = 0; i < voltage.size(); ++i) {
            if (time[i] >= start_time && time[i] <= end_time) {
                if (voltage[i] > maxVoltage) {
                    maxVoltage = voltage[i];
                }
            }
        }
        double threshold = maxVoltage * 0.7;
        return threshold;
    }

    vector<Beat> findRPeaks(const ECGRecord& record) const { // Nurettin Arda
        vector<Beat> beats;

        if (record.time.empty()) {
            return beats;
        }

        double startTime = record.time.front();
        double endTime = record.time.back();

        for (double currentTime = startTime; currentTime < endTime; currentTime += STEP_SIZE) {

            double intervalEnd = min(currentTime + INTERVAL_SIZE, endTime);

            double threshold = findThreshold(record, currentTime, intervalEnd);

            for (size_t i = 1; i + 1 < record.time.size(); ++i) {
                double tVal = record.time[i];
                if (tVal >= currentTime && tVal <= intervalEnd) {

                    if (record.voltage[i] > threshold &&
                        record.voltage[i] > record.voltage[i - 1] &&
                        record.voltage[i] > record.voltage[i + 1]) {

                        if (!beats.empty()) {
                            double timeSinceLastBeat = tVal - beats.back().rPeakTime;
                            if (timeSinceLastBeat < MIN_RR_INTERVAL*0.1) {
                                continue;
                            }
                        }

                        beats.push_back(Beat{ tVal });
                    }
                }
            }
        }
        return beats;
    }

    void analyzeECGData(const string& filename, const ECGRecord& record) const { //Salih Eren
        vector<pair<double, double>> tachycardiaIntervals;
        vector<pair<double, double>> bradycardiaIntervals;
        vector<pair<double, double>> normalIntervals;

        vector<Beat> beats = findRPeaks(record);
         
        for (size_t i = 0; i + 1 < beats.size(); ++i) {
            double rrInterval = beats[i + 1].rPeakTime - beats[i].rPeakTime;
            string classification = classifyBeat(rrInterval);

            if (classification == "Tachycardia") {
                tachycardiaIntervals.emplace_back(beats[i].rPeakTime, beats[i + 1].rPeakTime);
            }
            else if (classification == "Bradycardia") {
                bradycardiaIntervals.emplace_back(beats[i].rPeakTime, beats[i + 1].rPeakTime);
            }
            else if (classification == "Normal") {
                normalIntervals.emplace_back(beats[i].rPeakTime, beats[i + 1].rPeakTime);
            }
        }

        string outputPrefix = filename.substr(0, filename.find_last_of("."));

        writeAnalysisResult(generateOutputFilename(outputPrefix, "Tachycardia"), tachycardiaIntervals);
        writeAnalysisResult(generateOutputFilename(outputPrefix, "Bradycardia"), bradycardiaIntervals);
        writeAnalysisResult(generateOutputFilename(outputPrefix, "Normal"), normalIntervals);
    }

	void writeAnalysisResult(const string& filename, const vector<pair<double, double>>& intervals) const { //Salih Eren
        ofstream outfile(filename);
        if (!outfile.is_open()) {
            throw runtime_error("File can not be opened: " + filename);
        }

        outfile << fixed << setprecision(6);
        for (const auto& interval : intervals) {
            outfile << interval.first << " " << interval.second << endl;
        }

        outfile.close();
    }

public:

    void processFiles(const vector<string>& inputFiles) const { //Ceyda
        for (const string& file : inputFiles) {
            ECGRecord record = readECGFile(file);
            analyzeECGData(file, record);
        }
    }

    string generateOutputFilename(const string& fileName, const string& conditionType) const { //Ceyda
        string prefix = fileName.substr(0, fileName.find_last_of("."));
        return prefix + "-" + conditionType + ".txt";
    }

    string generateCombinedOutputFilename(const vector<string>& inputFiles, const string& conditionType) { //Ceyda
        string combinedFilename = conditionType + "-Person";
        for (const auto& file : inputFiles) {
            string baseFilename = file.substr(0, file.find_last_of("."));
            size_t firstDigitPos = baseFilename.find_first_of("0123456789");
            if (firstDigitPos != string::npos) {
                string personNumber = baseFilename.substr(firstDigitPos);
                combinedFilename += "-" + personNumber;
            }
        }
        combinedFilename += ".txt";
        return combinedFilename;
    }

    void combineResults(const string& outputFilename, const vector<string>& inputFiles) const { //Hilal Basak, Nurettin Arda
        ofstream outfile(outputFilename);
        if (!outfile.is_open()) {
            throw runtime_error("File can not be opened.");
        }

        bool isFirstBlock = true;
        for (const auto& file : inputFiles) {
            ifstream infile(file);
            if (!infile.is_open()) {
                cerr << "Error: File can not be opened." << endl;
                continue;
            }

            if (!isFirstBlock) {
                outfile << "**************\n";
            }
            isFirstBlock = false;

            string line;
            while (getline(infile, line)) {
                outfile << line << endl;
            }

            infile.close();
        }

        outfile.close();
    }
};

int main() { //Hilal Basak
    ECGProcessor processor;

    vector<string> inputFiles = {"Person1.txt","Person3.txt"};

    processor.processFiles(inputFiles);

    vector<string> conditions = { "Normal", "Tachycardia", "Bradycardia" };
    
    for (const auto& condition : conditions) {
        vector<string> conditionFiles;
        for (const auto& file : inputFiles) {
            conditionFiles.push_back(processor.generateOutputFilename(file, condition));
        }

        processor.combineResults(processor.generateCombinedOutputFilename(inputFiles,condition), conditionFiles);
    }
    cout << "ECG analysis completed." << endl;
    return 0;
}