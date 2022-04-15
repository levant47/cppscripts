#include <windows.h>

void copy_memory(const char* source, int size, char* destination)
{
    for (int i = 0; i < size; i++)
    {
        destination[i] = source[i];
    }
}

struct FindLastIndexOfResult
{
    bool success;
    int index;
};

FindLastIndexOfResult find_last_index_of_c_string(char c, const char* string)
{
    auto found = false;
    int index;
    for (int i = 0; string[i] != '\0'; i++)
    {
        if (string[i] == c)
        {
            index = i;
            found = true;
        }
    }
    FindLastIndexOfResult result;
    result.success = found;
    result.index = index;
    return result;
}

int int_to_string(int value, char* buffer)
{
    int size = 0;
    do
    {
        buffer[size] = (value % 10) + '0';
        value /= 10;
        size++;
    }
    while (value != 0);

    for (int i = 0; i < size / 2; i++)
    {
        char temp = buffer[i];
        buffer[i] = buffer[size - i - 1];
        buffer[size - i - 1] = temp;
    }

    return size;
}

int c_string_length(const char* string)
{
    int length = 0;
    while (string[length] != '\0')
    {
        length++;
    }
    return length;
}

struct ParseIntResult
{
    bool success;
    int value;

    static ParseIntResult fail()
    {
        ParseIntResult result;
        result.success = false;
        return result;
    }
};

ParseIntResult parse_int_from_c_string(char* source)
{
    if (source[0] == '\0')
    {
        return ParseIntResult::fail();
    }
    int value = 0;
    int i = 0;
    do
    {
        if (!(source[i] >= '0' && source[i] <= '9'))
        {
            return ParseIntResult::fail();
        }
        value = value * 10 + source[i] - '0';
        i++;
    }
    while (source[i] != '\0');

    ParseIntResult result;
    result.success = true;
    result.value = value;
    return result;
}

struct SplitParameters
{
    int segment_size;
    const char* target_path;
};

struct ParseSplitParametersResult
{
    bool success;
    SplitParameters split_parameters;

    static ParseSplitParametersResult fail()
    {
        ParseSplitParametersResult result;
        result.success = false;
        return result;
    }
};

ParseSplitParametersResult parse_split_parameters(char* source)
{
    int index = 0;

    while (source[index] != ' ')
    {
        if (source[index] == '\0') { return ParseSplitParametersResult::fail(); }
        index++;
    }
    while (source[index] == ' ') { index++; }
    if (source[index] == '\0') { return ParseSplitParametersResult::fail(); }

    int segment_size = 0;
    bool segment_size_parsed = false;
    while (source[index] >= '0' && source[index] <= '9')
    {
        segment_size = segment_size * 10 + source[index] - '0';
        index++;
        segment_size_parsed = true;
    }
    if (!segment_size_parsed || source[index] == '0') { return ParseSplitParametersResult::fail(); }

    while (source[index] == ' ') { index++; }
    if (source[index] == '\0') { return ParseSplitParametersResult::fail(); }

    auto target_path = source + index;
    while (source[index] != '\0')
    {
        if (source[index] == ' ') { return ParseSplitParametersResult::fail(); }
        index++;
    }

    ParseSplitParametersResult result;
    result.success = true;
    result.split_parameters.segment_size = segment_size;
    result.split_parameters.target_path = target_path;
    return result;
}

void SplitFile(int segment_size, const char* target_file_name, HANDLE target_file_handle)
{
    auto target_file_name_length = c_string_length(target_file_name);

    auto buffer_size = segment_size;
    auto buffer = VirtualAlloc(NULL, buffer_size, MEM_COMMIT, PAGE_READWRITE);

    DWORD bytes_read;
    int segment_index = 1;
    do
    {
        ReadFile(target_file_handle, buffer, buffer_size, &bytes_read, NULL);
        if (bytes_read == 0)
        {
            break;
        }

        char segment_file_name[256];
        copy_memory(target_file_name, target_file_name_length, segment_file_name);
        segment_file_name[target_file_name_length] = '.';
        auto index_length = int_to_string(segment_index, segment_file_name + target_file_name_length + 1);
        auto segment_file_name_length = target_file_name_length + 1 + index_length;
        segment_file_name[segment_file_name_length] = '\0';

        auto segment_file_handle = CreateFileA(
            segment_file_name,
            GENERIC_WRITE,
            FILE_SHARE_READ,
            NULL,
            CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL,
            NULL
        );

        WriteFile(
            segment_file_handle,
            buffer,
            bytes_read,
            NULL,
            NULL
        );

        CloseHandle(segment_file_handle);

        segment_index++;
    }
    while (bytes_read == buffer_size);

    VirtualFree(buffer, 0, MEM_RELEASE);
}

int main()
{
    auto stdout = GetStdHandle(STD_OUTPUT_HANDLE);
    if (stdout == NULL || stdout == INVALID_HANDLE_VALUE)
    {
        return 2;
    }

    auto command_line = GetCommandLineA();
    auto parse_split_parameters_result = parse_split_parameters(command_line);
    if (!parse_split_parameters_result.success)
    {
        char message[] = "Failed to parse CLI arguments, usage: split <segment size in bytes> <file path>\n";
        WriteConsole(stdout, message, sizeof(message), NULL, NULL);
        return 1;
    }
    auto split_parameters = parse_split_parameters_result.split_parameters;

    auto target_file_handle = CreateFileA(
        split_parameters.target_path,
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );
    if (target_file_handle == INVALID_HANDLE_VALUE)
    {
        char message[] = "Failed to open the target file\n";
        WriteConsole(stdout, message, sizeof(message), NULL, NULL);
        return 1;
    }

    auto target_file_name = split_parameters.target_path;
    auto maybe_last_slash_index_in_target_path = find_last_index_of_c_string('\\', split_parameters.target_path);
    if (maybe_last_slash_index_in_target_path.success)
    {
        target_file_name += maybe_last_slash_index_in_target_path.index + 1;
    }
    SplitFile(split_parameters.segment_size, target_file_name, target_file_handle);

    CloseHandle(target_file_handle);

    char final_message[] = "Done\n";
    WriteConsole(stdout, final_message, sizeof(final_message), NULL, NULL);

    return 0;
}
