#include <iostream>
#include <fstream>
#include <map>
#include <vector>
#include <string.h>
#include <cstdlib>


using namespace std;

struct Target;

vector<Target *> targets;
map<string, size_t> targets_names;
map<string, string> variables_map;
Target *last_target;

struct Target
{
    size_t index;
    string name;
    vector<size_t> dependencies;
    vector<string *> actions;
    bool was_initialized;

    Target(string name) : name(name)
    {
        index = targets.size();
        targets.push_back(this);
        targets_names[name] = index;
        was_initialized = false;
    }

    ~Target()
    {
        for (size_t i = 0; i < actions.size(); i++)
            delete actions[i];
    }
};


int error_line_number = -1;
string error_description = "";

void setError(string message, int line_number)
{
    error_line_number = line_number;
    error_description = message;
}

bool hasError()
{
    return (error_description != "");
}

void cleanTargets()
{
    for (size_t i = 0; i < targets.size(); i++)
        delete targets[i];
}

void tryExitWithError()
{
    if (hasError())
    {
        cleanTargets();
        if (error_line_number != -1)
            cout << "Error line: " << error_line_number << endl;
        cout << error_description << endl;
        exit(EXIT_FAILURE);
    }
}

void addAction(const string &s, int i)
{
    if (targets.empty())
    {
        setError("Action come before target.", i);
        return;
    }

    // resolve variables

    string *explicit_s = new string();
    for (size_t j = 0; j < s.size(); j++)
    {
        if (s.find('$', j) == j)
        {
            size_t closing_bracket_pos = s.find(')', j);
            if (s[j + 1] != '(' || closing_bracket_pos == -1)
            {
                setError("Variable reference is wrong formatted.", i);
                return;
            }
            if (variables_map.find(s.substr(j + 2, closing_bracket_pos - j - 2)) == variables_map.end())
            {
                setError("Unknown variable  \"" + s.substr(j + 2, closing_bracket_pos - j - 2) + "\".", i);
                return;
            }
            (*explicit_s) += variables_map[s.substr(j + 2, closing_bracket_pos - j - 2)];
            j = closing_bracket_pos;
        }
        else
            *explicit_s += s[j];
    }
    last_target->actions.push_back(explicit_s);
}

Target *getOrCreateTargetByName(const string &name)
{
    if (targets_names.find(name) == targets_names.end())
        return new Target(name);          // colon existence was checked outside.
    else
        return targets[targets_names[name]];
}

void addTarget(const string &s, int i)
{
    last_target = getOrCreateTargetByName(s.substr(0, s.find(':')));
    if (last_target->was_initialized)
    {
        setError("target \"" + last_target->name + "\" has multiple definitions.", i);
        return;
    }
    last_target->was_initialized = true;

    for (size_t j = s.find(':') + 1; j >= 0 && j < s.size(); j++)
    {
        if (s[j] == ' ')
        {
            j = s.find_first_not_of(' ', j) - 1;
            continue;
        }
        Target *dependency_target;
        size_t next_space = s.find(' ', j);
        if (next_space == -1)
            dependency_target = getOrCreateTargetByName(s.substr(j));
        else
            dependency_target = getOrCreateTargetByName(s.substr(j, next_space - j));
        last_target->dependencies.push_back(dependency_target->index);
        if (next_space == -1)
            break;
        j = next_space;
    }
}

void addVariable(string &s, int i)
{
    size_t equal_sign_pos = s.find('=');
    if (variables_map.find(s.substr(0, equal_sign_pos)) != variables_map.end())
    {
        setError("Variable \"" + s.substr(0, equal_sign_pos) + "\" definition is not unique.", i);
        return;
    }
    variables_map[s.substr(0, equal_sign_pos)] = s.substr(equal_sign_pos + 1, s.size() - equal_sign_pos - 1);
}

void readInput()
{
    bool variables_finished = false;
    ifstream input_stream("a.in");
    if (!input_stream)
    {
        setError("Input file is bad. File \"a.in\" is considered to be an input file.", -1);
        return;
    }
    string s = "";
    int i = 0;
    while (getline(input_stream, s))
    {
        if (hasError())
            return;
        i++;
        if (s.find_first_not_of(' ') == -1)
            continue;
        if (s.size() > 0 && s[0] == ' ')
        {
            variables_finished = true;
            addAction(s, i);
            continue;
        }
        size_t equal_sign_pos = s.find('=');
        size_t colon_sign_pos = s.find(':');
        if (equal_sign_pos == -1 && colon_sign_pos == -1)
        {
            setError("Unknown type of line.", i);
            return;
        }
        if (colon_sign_pos == -1 || equal_sign_pos < colon_sign_pos)
        {
            if (variables_finished)
            {
                setError("Variable is coming after some targets.", i);
                return;
            }
            addVariable(s, i);
            continue;
        }
        variables_finished = true;
        addTarget(s, i);
    }
}

void printTargets()
{
    if (error_description != "")
    {
        cout << "Error line: " << error_line_number << endl;
        cout << error_description << endl;
    }
    for (size_t i = 0; i < targets.size(); i++)
    {
        Target *t = targets[i];
        cout << "Target: " << t->index << " " << t->name << endl;
        for (size_t j = 0; j < t->dependencies.size(); j++)
            cout << t->dependencies[j] << " ";
        cout << endl;
    }
}

void deleteRepeatsInVector(vector<size_t> &ans, char *already_seen, size_t max_index, const vector<size_t> &origin)
{
    ans.clear();
    memset(already_seen, 0, max_index);
    for (size_t j = 0; j < origin.size(); j++)
        if (!already_seen[origin[j]])
        {
            already_seen[origin[j]] = 1;
            ans.push_back(origin[j]);
        }
}

void deleteMultipleEdges()
{
    vector<size_t> temp;
    size_t max_index = targets.size();
    char already_seen[max_index];
    for (size_t i = 0; i < max_index; i++)
    {
        deleteRepeatsInVector(temp, already_seen, max_index, targets[i]->dependencies);
        targets[i]->dependencies = temp;
    }
}

void runActions(Target *target)
{
    if (!target->was_initialized)
    {
        setError("Target \"" + target->name + "\"hasn't got definition.", -1);
        return;
    }
    cout << "Target \"" << target->name << "\" is executed:" << endl;
    if (target->actions.empty())
    {
        cout << "Finished empty target : \"" << target->name << "\". It hasn't got any actions." << endl;
        return;
    }
    string actions = (*target->actions[0]);
    for (size_t i = 1; i < target->actions.size(); i++)
    {
        //cout << "action: " << i << "  " << target->actions[i]->c_str() << endl;           //useful debug info
        actions += " && " + (*target->actions[i]);
    }
    int return_code = system(actions.c_str());
    cout << "Finished target : \"" << target->name << "\" with return code " << return_code << endl;
    if (return_code != 0)
        setError("Error in executing target's action.", -1);
}

void innerDFS(Target *target, char *visited, bool wantRunActions)
{
    if (hasError())
        return;
    visited[target->index] = 1;
    for (size_t i = 0; i < target->dependencies.size(); i++)
    {
        if (!wantRunActions && visited[target->dependencies[i]] == 1)
        {
            setError("There is a mutual dependency between \"" + target->name + "\" and \"" + targets[target->dependencies[i]]->name + "\"", -1);
            return;
        }
        if (!visited[target->dependencies[i]])
            innerDFS(targets[target->dependencies[i]], visited, wantRunActions);
        if (hasError())
            return;
    }
    visited[target->index] = 2;
    if (wantRunActions)
        runActions(target);
}

void outerDFS(Target *general_target, bool wantRunActions)
{
    size_t n = targets.size();
    char visited[n];
    memset(visited, 0, n);
    innerDFS(general_target, visited, wantRunActions);
}

Target *getGeneralTarget(const char *name)
{
    if (targets_names.find(name) == targets_names.end())
        setError("Can't find general target with such name.", -1);
    return targets[targets_names[name]];
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        setError("Usage: my_make main_target_name.\n File \"a.in\" is considered to be an input file.", -1);
        tryExitWithError();
    }

    readInput();
    tryExitWithError();

    //printTargets();                   //for debug using

    deleteMultipleEdges();

    Target *general_target = getGeneralTarget(argv[1]);
    outerDFS(general_target, false);
    tryExitWithError();

    outerDFS(general_target, true);
    tryExitWithError();

    cleanTargets();
    cout << "All work was finished." << endl;

    return 0;
}