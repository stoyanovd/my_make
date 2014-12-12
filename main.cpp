#include <iostream>
#include <fstream>
#include <map>
#include <vector>
#include <string>
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
    vector<string> actions;
    bool was_initialized;

    Target(string name) : name(name)
    {
        index = targets.size();
        targets.push_back(this);
        targets_names[name] = index;
        was_initialized = false;
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

void resolveVariables(string &explicit_s, const string &s, int i)
{
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
            explicit_s += variables_map[s.substr(j + 2, closing_bracket_pos - j - 2)];
            j = closing_bracket_pos;
        }
        else
            explicit_s += s[j];
    }
}

void addAction(const string &s, int i)
{
    if (targets.empty())
    {
        setError("Action come before target.", i);
        return;
    }

    last_target->actions.push_back(string());
    resolveVariables(last_target->actions[last_target->actions.size() - 1], s, i);
}

Target *getOrCreateTargetByName(const string &name)
{
    if (targets_names.find(name) == targets_names.end())
        return new Target(name);          // colon existence was checked outside.
    else
        return targets[targets_names[name]];
}

void addDependencies(const string &s)
{
    for (size_t j = 0; j >= 0 && j < s.size(); j++)
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

void addTarget(const string &s, int i)
{
    last_target = getOrCreateTargetByName(s.substr(0, s.find(':')));
    if (last_target->was_initialized)
    {
        setError("Target \"" + last_target->name + "\" has multiple definitions.", i);
        return;
    }
    last_target->was_initialized = true;

    addDependencies(s.substr(s.find(':') + 1));
}

void cleanExcessChars(string &s)
{
    string only_true_chars = "";

    for (size_t j = 0; j < s.size(); j++)
        if (s[j] >= ' ')
            only_true_chars += s[j];
        else if (s[j] == '\t')
            only_true_chars += ' ';

    s = only_true_chars;
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

void readInput(const char *file_name)
{
    bool variables_finished = false;
    ifstream input_stream(file_name);
    if (!input_stream.is_open())
    {
        setError("Input file is bad.", -1);
        return;
    }
    string s = "";
    int i = 0;
    while (getline(input_stream, s))
    {
        if (hasError())
            return;
        i++;
        cleanExcessChars(s);
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

void deleteRepeatsInVector(vector<size_t> &ans, size_t max_index, const vector<size_t> &origin)
{
    ans.clear();
    vector<bool> already_seen(max_index, 0);
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
    const size_t max_index = targets.size();
    for (size_t i = 0; i < max_index; i++)
    {
        deleteRepeatsInVector(temp, max_index, targets[i]->dependencies);
        targets[i]->dependencies = temp;
    }
}

void runActions(Target *target)
{
    if (!target->was_initialized)
    {
        setError("Target \"" + target->name + "\" hasn't got definition.", -1);
        return;
    }
    cout << "Target \"" << target->name << "\" is executed:" << endl;
    if (target->actions.empty())
    {
        cout << "Finished empty target : \"" << target->name << "\". It hasn't got any actions." << endl;
        return;
    }
    string actions = target->actions[0];
    for (size_t i = 1; i < target->actions.size(); i++)
    {
        //cout << "action: " << i << "  " << target->actions[i]->c_str() << endl;           //useful debug info
        actions += " && " + target->actions[i];
    }
    int return_code = system(actions.c_str());
    cout << "Finished target : \"" << target->name << "\" with return code " << return_code << endl;
    if (return_code != 0)
        setError("Error in executing target's action.", -1);
}

void unbentDFS(Target *general_target, bool wantRunActions)
{
    const size_t n = targets.size();
    vector<char> visited(n, 0);
    vector<pair<Target *, size_t> > recursive_stack;
    recursive_stack.push_back(make_pair(general_target, 0));
    while (!recursive_stack.empty())
    {
        if (hasError())
            return;
        Target *v = recursive_stack.back().first;
        size_t i = recursive_stack.back().second;
        recursive_stack.pop_back();
        if (i >= v->dependencies.size())
        {
            visited[v->index] = 2;
            if (wantRunActions)
                runActions(v);
            continue;
        }
        Target *to_add = targets[v->dependencies[i]];
        recursive_stack.push_back(make_pair(v, i + 1));
        if (visited[to_add->index] == 1)
        {
            setError("There is a mutual dependency between \"" + v->name + "\" and \"" + to_add->name + "\"", -1);
            return;
        }
        if (visited[to_add->index] == 0)
        {
            visited[to_add->index] = 1;
            recursive_stack.push_back(make_pair(to_add, 0));
        }
    }
}

Target *getGeneralTarget(const char *name)
{
    if (targets_names.find(name) == targets_names.end())
        setError("Can't find general target with such name.", -1);
    return targets[targets_names[name]];
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        setError("Usage:   my_make   [MAIN TARGET]  [FILE NAME]", -1);
        tryExitWithError();
    }

    readInput(argv[2]);
    tryExitWithError();

    //printTargets();                   //for debug using

    deleteMultipleEdges();

    Target *general_target = getGeneralTarget(argv[1]);
    //printTargets();                   //for debug using

    unbentDFS(general_target, false);
    tryExitWithError();

    unbentDFS(general_target, true);
    tryExitWithError();

    cleanTargets();
    cout << "All work was finished." << endl;

    return 0;
}