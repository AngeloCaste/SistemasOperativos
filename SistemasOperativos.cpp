#include <iostream>
#include <vector>
#include <queue>
#include <algorithm>
#include <chrono>
#include <iomanip>
#include <numeric>
#include <fstream>
#include <sstream>
#include <string>

using namespace std;
using namespace std::chrono;

struct Proceso {
    string nombre;
    int ti;    // Tiempo inicial
    int t;     // Tiempo de actividad (duración)
    int tf;    // Tiempo final
    int T;     // T = tf - ti
    float E;   // E = T - t
    float I;   // I = t / T
};

vector<Proceso> leerCSV(const string& filename) {
    vector<Proceso> procesos;
    ifstream file(filename);
    string line;

    if (!file.is_open()) {
        cerr << "Error al abrir el archivo CSV. Se usará entrada manual." << endl;
        return procesos;
    }

    while (getline(file, line)) {
        stringstream ss(line);
        string nombre, valores;
        char discard;
        int ti, t;

        getline(ss, nombre, ',');
        nombre.erase(remove(nombre.begin(), nombre.end(), '\"'), nombre.end());
        getline(ss, valores);

        // Limpiar y parsear los valores (ti, t)
        valores.erase(remove(valores.begin(), valores.end(), ' '), valores.end());
        valores.erase(remove(valores.begin(), valores.end(), '('), valores.end());
        valores.erase(remove(valores.begin(), valores.end(), ')'), valores.end());
        valores.erase(remove(valores.begin(), valores.end(), '\"'), valores.end());

        stringstream valss(valores);
        valss >> ti >> discard >> t;

        procesos.push_back({nombre, ti, t, 0, 0, 0.0f, 0.0f});
    }

    file.close();
    return procesos;
}

vector<Proceso> entradaManual() {
    vector<Proceso> procesos;
    int n;
    
    cout << "Ingrese el numero de procesos: ";
    cin >> n;
    
    for (int i = 0; i < n; ++i) {
        Proceso p;
        cout << "\nProceso " << (i+1) << ":" << endl;
        
        // Generar nombre automático (A, B, ..., Z, A1, B1, ...)
        if (i < 26) {
            p.nombre = string(1, 'A' + i);
        } else {
            p.nombre = string(1, 'A' + (i % 26)) + to_string(i / 26);
        }
        
        cout << "Nombre generado: " << p.nombre << endl;
        cout << "Ingrese tiempo inicial (ti): ";
        cin >> p.ti;
        cout << "Ingrese tiempo de actividad (t): ";
        cin >> p.t;
        
        procesos.push_back(p);
    }
    
    return procesos;
}

void calcularMetricas(vector<Proceso>& procesos) {
    for (auto& p : procesos) {
        p.T = p.tf - p.ti;
        p.E = p.T - p.t;
        p.I = static_cast<float>(p.t) / p.T;
    }
}

void imprimirProcesos(const vector<Proceso>& procesos) {
    cout << left << setw(10) << "Proceso" << setw(10) << "ti" << setw(10) << "t" 
         << setw(10) << "tf" << setw(10) << "T" << setw(15) << "E (T - t)" 
         << setw(15) << "I (t/T)" << endl;
    cout << string(80, '-') << endl;

    for (const auto& p : procesos) {
        cout << left << setw(10) << p.nombre << setw(10) << p.ti << setw(10) << p.t 
             << setw(10) << p.tf << setw(10) << p.T 
             << setw(15) << fixed << setprecision(2) << p.E 
             << setw(15) << p.I << endl;
    }
}

void calcularPromedios(const vector<Proceso>& procesos) {
    float avg_T = 0, avg_E = 0, avg_I = 0;
    
    for (const auto& p : procesos) {
        avg_T += p.T;
        avg_E += p.E;
        avg_I += p.I;
    }
    
    int n = procesos.size();
    avg_T /= n;
    avg_E /= n;
    avg_I /= n;
    
    cout << "\nPromedios:" << endl;
    cout << "T promedio: " << fixed << setprecision(2) << avg_T << endl;
    cout << "E promedio: " << avg_E << endl;
    cout << "I promedio: " << avg_I << endl;
}

vector<Proceso> FIFO(vector<Proceso> procesos) {
    auto start = high_resolution_clock::now();
    
    // Ordenar por tiempo inicial (ti)
    sort(procesos.begin(), procesos.end(), [](const Proceso& a, const Proceso& b) {
        return a.ti < b.ti;
    });
    
    // Calcular tiempos finales
    int tiempo_actual = 0;
    for (auto& p : procesos) {
        if (p.ti > tiempo_actual) {
            tiempo_actual = p.ti;
        }
        p.tf = tiempo_actual + p.t;
        tiempo_actual = p.tf;
    }
    
    calcularMetricas(procesos);
    
    auto stop = high_resolution_clock::now();
    auto duration = duration_cast<microseconds>(stop - start);
    
    cout << "\nFIFO - Tiempo de ejecucion: " << duration.count() << " microsegundos" << endl;
    return procesos;
}

vector<Proceso> LIFO(vector<Proceso> procesos) {
    auto start = high_resolution_clock::now();
    
    // Ordenar por tiempo inicial (ti) en orden descendente
    sort(procesos.begin(), procesos.end(), [](const Proceso& a, const Proceso& b) {
        return a.ti > b.ti;
    });
    
    // Calcular tiempos finales
    int tiempo_actual = 0;
    for (auto& p : procesos) {
        if (p.ti > tiempo_actual) {
            tiempo_actual = p.ti;
        }
        p.tf = tiempo_actual + p.t;
        tiempo_actual = p.tf;
    }
    
    // Restaurar orden original para presentación
    sort(procesos.begin(), procesos.end(), [](const Proceso& a, const Proceso& b) {
        return a.ti < b.ti;
    });
    
    calcularMetricas(procesos);
    
    auto stop = high_resolution_clock::now();
    auto duration = duration_cast<microseconds>(stop - start);
    
    cout << "\nLIFO - Tiempo de ejecucion: " << duration.count() << " microsegundos" << endl;
    return procesos;
}

vector<Proceso> RoundRobin(vector<Proceso> procesos, int quantum) {
    auto start = high_resolution_clock::now();
    
    // Ordenar por tiempo inicial (ti)
    sort(procesos.begin(), procesos.end(), [](const Proceso& a, const Proceso& b) {
        return a.ti < b.ti;
    });
    
    queue<Proceso*> cola;
    vector<int> tiempo_restante;
    
    for (auto& p : procesos) {
        tiempo_restante.push_back(p.t);
    }
    
    int tiempo_actual = 0;
    int procesos_completados = 0;
    int n = procesos.size();
    int indice = 0;
    
    while (procesos_completados < n) {
        // Agregar procesos que han llegado a la cola
        while (indice < n && procesos[indice].ti <= tiempo_actual) {
            cola.push(&procesos[indice]);
            indice++;
        }
        
        if (!cola.empty()) {
            Proceso* actual = cola.front();
            cola.pop();
            
            int indice_actual = distance(&procesos[0], actual);
            
            if (tiempo_restante[indice_actual] > quantum) {
                tiempo_actual += quantum;
                tiempo_restante[indice_actual] -= quantum;
                
                // Agregar procesos que han llegado mientras se ejecutaba
                while (indice < n && procesos[indice].ti <= tiempo_actual) {
                    cola.push(&procesos[indice]);
                    indice++;
                }
                
                cola.push(actual); // Volver a poner en la cola
            } else {
                tiempo_actual += tiempo_restante[indice_actual];
                tiempo_restante[indice_actual] = 0;
                actual->tf = tiempo_actual;
                procesos_completados++;
            }
        } else {
            tiempo_actual++;
        }
    }
    
    calcularMetricas(procesos);
    
    auto stop = high_resolution_clock::now();
    auto duration = duration_cast<microseconds>(stop - start);
    
    cout << "\nRound Robin (Q=" << quantum << ") - Tiempo de ejecucion: " 
         << duration.count() << " microsegundos" << endl;
    return procesos;
}

void compararMetodos(const vector<Proceso>& fifo, const vector<Proceso>& lifo, const vector<Proceso>& rr) {
    auto calcularPromedio = [](const vector<Proceso>& procesos, char metric) {
        float suma = 0;
        for (const auto& p : procesos) {
            switch (metric) {
                case 'T': suma += p.T; break;
                case 'E': suma += p.E; break;
                case 'I': suma += p.I; break;
            }
        }
        return suma / procesos.size();
    };
    
    float fifo_T = calcularPromedio(fifo, 'T');
    float fifo_E = calcularPromedio(fifo, 'E');
    float fifo_I = calcularPromedio(fifo, 'I');
    
    float lifo_T = calcularPromedio(lifo, 'T');
    float lifo_E = calcularPromedio(lifo, 'E');
    float lifo_I = calcularPromedio(lifo, 'I');
    
    float rr_T = calcularPromedio(rr, 'T');
    float rr_E = calcularPromedio(rr, 'E');
    float rr_I = calcularPromedio(rr, 'I');
    
    cout << "\nComparacion de metodos (promedios):" << endl;
    cout << "Metrica |   FIFO   |   LIFO   | Round Robin" << endl;
    cout << "-------------------------------------------" << endl;
    cout << fixed << setprecision(2);
    cout << "T       | " << setw(8) << fifo_T << " | " << setw(8) << lifo_T << " | " << setw(8) << rr_T << endl;
    cout << "E       | " << setw(8) << fifo_E << " | " << setw(8) << lifo_E << " | " << setw(8) << rr_E << endl;
    cout << "I       | " << setw(8) << fifo_I << " | " << setw(8) << lifo_I << " | " << setw(8) << rr_I << endl;
    
    // Determinar el mejor método basado en T promedio (menor es mejor)
    string mejor_metodo;
    float menor_T = min({fifo_T, lifo_T, rr_T});
    
    if (menor_T == fifo_T) mejor_metodo = "FIFO";
    else if (menor_T == lifo_T) mejor_metodo = "LIFO";
    else mejor_metodo = "Round Robin";
    
    cout << "\nEl mejor metodo en terminos de T promedio es: " << mejor_metodo << endl;
}

int main() {
    vector<Proceso> procesos;
    int quantum;
    int opcion;
    
    cout << "SELECCIONE MODO DE ENTRADA:\n";
    cout << "1. Leer datos desde archivo CSV\n";
    cout << "2. Ingresar datos manualmente\n";
    cout << "Opcion: ";
    cin >> opcion;
    
    if (opcion == 1) {
        string filename;
        cout << "Ingrese el nombre del archivo CSV: ";
        cin >> filename;
        procesos = leerCSV(filename);
        
        if (procesos.empty()) {
            cout << "No se pudieron leer datos del CSV. Usando entrada manual.\n";
            procesos = entradaManual();
        }
    } else {
        procesos = entradaManual();
    }
    
    cout << "\nIngrese el quantum para Round Robin0: ";
    cin >> quantum;
    
    // Ejecutar los tres métodos
    vector<Proceso> fifo = FIFO(procesos);
    imprimirProcesos(fifo);
    calcularPromedios(fifo);
    
    vector<Proceso> lifo = LIFO(procesos);
    imprimirProcesos(lifo);
    calcularPromedios(lifo);
    
    vector<Proceso> rr = RoundRobin(procesos, quantum);
    imprimirProcesos(rr);
    calcularPromedios(rr);
    
    // Comparar métodos
    compararMetodos(fifo, lifo, rr);
    
    return 0;
}