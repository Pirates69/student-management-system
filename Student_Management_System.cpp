#include <iostream>
#include <fstream>
#include <cstdio>
#include <limits>
#include <cstring>
#include <string>

using namespace std;

const char DATA_FILE[] = "student.dat";
const char TEMP_FILE[] = "temp.dat";

int readInt(const string& prompt) {
    int value;
    while (true) {
        cout << prompt;
        if (cin >> value) {
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            return value;
        }
        cout << "Invalid input. Please enter a whole number.\n";
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
    }
}

float readMarks() {
    float marks;
    while (true) {
        cout << "Enter Marks       : ";
        if (cin >> marks && marks >= 0.0f && marks <= 100.0f) {
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            return marks;
        }
        cout << "Invalid marks. Enter a value from 0 to 100.\n";
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
    }
}

class Student {
private:
    int roll{};
    char name[50]{};
    char course[50]{};
    float marks{};

public:
    void input(bool askRoll = true) {
        if (askRoll) {
            roll = readInt("Enter Roll Number : ");
        }

        cout << "Enter Name        : ";
        cin.getline(name, sizeof(name));
        while (cin.fail()) {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Name is too long. Enter up to 49 characters: ";
            cin.getline(name, sizeof(name));
        }

        cout << "Enter Course      : ";
        cin.getline(course, sizeof(course));
        while (cin.fail()) {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Course is too long. Enter up to 49 characters: ";
            cin.getline(course, sizeof(course));
        }

        marks = readMarks();
    }

    void display() const {
        cout << "\nRoll No : " << roll
             << "\nName    : " << name
             << "\nCourse  : " << course
             << "\nMarks   : " << marks << '\n';
    }

    int getRoll() const { return roll; }
    float getMarks() const { return marks; }
    void setRoll(int value) { roll = value; }

    // Write each field separately rather than dumping the whole object,
    // which can include compiler-specific padding bytes.
    bool writeTo(ostream& out) const {
        out.write(reinterpret_cast<const char*>(&roll), sizeof(roll));
        out.write(name, sizeof(name));
        out.write(course, sizeof(course));
        out.write(reinterpret_cast<const char*>(&marks), sizeof(marks));
        return static_cast<bool>(out);
    }

    bool readFrom(istream& in) {
        in.read(reinterpret_cast<char*>(&roll), sizeof(roll));
        if (!in) return false;
        in.read(name, sizeof(name));
        in.read(course, sizeof(course));
        in.read(reinterpret_cast<char*>(&marks), sizeof(marks));
        return static_cast<bool>(in);
    }
};

bool rollExists(int roll) {
    ifstream file(DATA_FILE, ios::binary);
    Student s;
    while (s.readFrom(file)) {
        if (s.getRoll() == roll) return true;
    }
    return false;
}

void addStudent() {
    Student s;
    cout << "\n------ ADD STUDENT ------\n";
    int roll = readInt("Enter Roll Number : ");
    if (roll <= 0) {
        cout << "Roll number must be positive.\n";
        return;
    }
    if (rollExists(roll)) {
        cout << "A student with this roll number already exists.\n";
        return;
    }

    // Student::input normally reads the roll number too. Here the roll
    // is already checked, so input the remaining fields and set roll via
    // a dedicated helper below.
    s.setRoll(roll);
    s.input(false);

    ofstream file(DATA_FILE, ios::binary | ios::app);
    if (!file) {
        cout << "Error opening student data file.\n";
        return;
    }
    if (!s.writeTo(file)) {
        cout << "Error writing student record.\n";
        return;
    }
    cout << "Student added successfully.\n";
}

void displayStudents() {
    ifstream file(DATA_FILE, ios::binary);
    if (!file) {
        cout << "\nNo records found.\n";
        return;
    }

    Student s;
    int count = 0;
    cout << "\n------ STUDENT RECORDS ------\n";
    while (s.readFrom(file)) {
        s.display();
        cout << "-----------------------------\n";
        ++count;
    }

    if (count == 0) cout << "No student records available.\n";
    else cout << "Total Students : " << count << '\n';
}

void searchStudent() {
    int roll = readInt("Enter Roll Number to Search : ");
    ifstream file(DATA_FILE, ios::binary);
    if (!file) {
        cout << "No records found.\n";
        return;
    }

    Student s;
    while (s.readFrom(file)) {
        if (s.getRoll() == roll) {
            cout << "\nStudent found:\n";
            s.display();
            return;
        }
    }
    cout << "Record not found.\n";
}

void updateStudent() {
    int roll = readInt("Enter Roll Number to Update : ");
    fstream file(DATA_FILE, ios::binary | ios::in | ios::out);
    if (!file) {
        cout << "No records found.\n";
        return;
    }

    Student s;
    while (true) {
        streampos recordStart = file.tellg();
        if (!s.readFrom(file)) break;

        if (s.getRoll() == roll) {
            cout << "\nEnter updated details (roll number stays the same).\n";
            s.input(false);

            file.clear();
            file.seekp(recordStart);
            if (!file || !s.writeTo(file)) {
                cout << "Error updating student record.\n";
                return;
            }
            cout << "Record updated successfully.\n";
            return;
        }
    }
    cout << "Record not found.\n";
}

void deleteStudent() {
    int roll = readInt("Enter Roll Number to Delete : ");
    ifstream in(DATA_FILE, ios::binary);
    if (!in) {
        cout << "No records found.\n";
        return;
    }

    ofstream out(TEMP_FILE, ios::binary | ios::trunc);
    if (!out) {
        cout << "Could not create temporary file.\n";
        return;
    }

    Student s;
    bool found = false;
    bool readError = false;

    while (s.readFrom(in)) {
        if (s.getRoll() == roll) {
            found = true;
        } else if (!s.writeTo(out)) {
            cout << "Error writing temporary file. Original file was kept.\n";
            in.close();
            out.close();
            std::remove(TEMP_FILE);
            return;
        }
    }

    if (!in.eof()) readError = true;
    in.close();
    out.close();

    if (readError || !out) {
        cout << "Error while processing records. Original file was kept.\n";
        std::remove(TEMP_FILE);
        return;
    }

    if (std::remove(DATA_FILE) != 0) {
        cout << "Could not replace the original data file. Original file was kept if still present.\n";
        std::remove(TEMP_FILE);
        return;
    }
    if (std::rename(TEMP_FILE, DATA_FILE) != 0) {
        cout << "Could not rename temporary file. Your records may be in temp.dat.\n";
        return;
    }

    if (found) cout << "Record deleted successfully.\n";
    else cout << "Record not found. No records were deleted.\n";
}

void showTopper() {
    ifstream file(DATA_FILE, ios::binary);
    if (!file) {
        cout << "No records found.\n";
        return;
    }

    Student s, top;
    bool found = false;
    while (s.readFrom(file)) {
        if (!found || s.getMarks() > top.getMarks()) {
            top = s;
            found = true;
        }
    }

    if (found) {
        cout << "\n------ CLASS TOPPER ------\n";
        top.display();
    } else {
        cout << "No student records available.\n";
    }
}

int main() {
    int choice;

    do {
        cout << "\n=================================\n"
             << "     STUDENT MANAGEMENT SYSTEM\n"
             << "=================================\n"
             << "1. Add Student\n"
             << "2. Display Students\n"
             << "3. Search Student\n"
             << "4. Update Student\n"
             << "5. Delete Student\n"
             << "6. Show Topper\n"
             << "7. Exit\n"
             << "=================================\n";

        choice = readInt("Enter choice : ");

        switch (choice) {
            case 1: addStudent(); break;
            case 2: displayStudents(); break;
            case 3: searchStudent(); break;
            case 4: updateStudent(); break;
            case 5: deleteStudent(); break;
            case 6: showTopper(); break;
            case 7: cout << "Exiting program.\n"; break;
            default: cout << "Invalid choice. Select 1 to 7.\n";
        }
    } while (choice != 7);

    return 0;
}
