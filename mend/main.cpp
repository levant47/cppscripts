#include <windows.h>

void copy_memory(const char* source, int size, char* destination)
{
    for (int i = 0; i < size; i++)
    {
        destination[i] = source[i];
    }
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

struct ParseCommandLineResult
{
    bool success;
    const char* target_path;

    static ParseCommandLineResult fail()
    {
        ParseCommandLineResult result;
        result.success = false;
        return result;
    }
};

ParseCommandLineResult parse_command_line(const char* source)
{
    int index = 0;

    // skip exe name
    while (source[index] != ' ')
    {
        if (source[index] == '\0') { return ParseCommandLineResult::fail(); }
        index++;
    }

    // skip whitespace after exe name
    while (source[index] == ' ') { index++; }

    // first argument is the target path
    auto target_path = source + index;

    // ensure that there is just one argument provided
    while (source[index] != '\0')
    {
        if (source[index] == ' ') { return ParseCommandLineResult::fail(); }
        index++;
    }

    ParseCommandLineResult result;
    result.success = true;
    result.target_path = target_path;
    return result;
}

// returns an error message or null if successful
const char* mend_files(const char* target_path)
{
    auto target_path_length = c_string_length(target_path);
    auto target_file_handle = CreateFileA(
        target_path,
        GENERIC_WRITE,
        FILE_SHARE_READ,
        NULL,
        OPEN_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );
    if (target_file_handle == INVALID_HANDLE_VALUE)
    {
        return "Failed to open target file\n";
    }

    int segment_index = 1;
    void* buffer = NULL;
    while (true)
    {
        char segment_path[256];
        copy_memory(target_path, target_path_length, segment_path);
        segment_path[target_path_length] = '.';
        auto index_length = int_to_string(segment_index, segment_path + target_path_length + 1);
        auto segment_path_length = target_path_length + 1 + index_length;
        segment_path[segment_path_length] = '\0';

        auto segment_file_handle = CreateFileA(
            segment_path,
            GENERIC_READ,
            FILE_SHARE_READ,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL
        );
        if (segment_file_handle == INVALID_HANDLE_VALUE)
        {
            break; // there are no more segment files left
        }

        auto buffer_size = GetFileSize(segment_file_handle, NULL);

        // allocate the buffer on the first pass
        if (buffer == NULL)
        {
            buffer = VirtualAlloc(NULL, buffer_size, MEM_COMMIT, PAGE_READWRITE);
            if (buffer == NULL)
            {
                return "Failed to allocate buffer\n";
            }
        }

        ReadFile(segment_file_handle, buffer, buffer_size, NULL, NULL);
        WriteFile(target_file_handle, buffer, buffer_size, NULL, NULL);

        CloseHandle(segment_file_handle);

        segment_index++;
    }

    VirtualFree(buffer, 0, MEM_RELEASE);
    CloseHandle(target_file_handle);

    return NULL;
}

int main()
{
    auto stdout = GetStdHandle(STD_OUTPUT_HANDLE);
    if (stdout == NULL || stdout == INVALID_HANDLE_VALUE)
    {
        return 2;
    }

    auto command_line = GetCommandLineA();
    auto parse_command_line_result = parse_command_line(command_line);
    if (!parse_command_line_result.success)
    {
        char message[] = "Failed to parse parameters, usage: mend <target file name>\n";
        WriteConsole(stdout, message, sizeof(message), NULL, NULL);
        return 1;
    }

    auto error_message = mend_files(parse_command_line_result.target_path);
    if (error_message != NULL)
    {
        WriteConsole(stdout, error_message, c_string_length(error_message), NULL, NULL);
        return 1;
    }

    char final_message[] = "Done\n";
    WriteConsole(stdout, final_message, sizeof(final_message), NULL, NULL);

    return 0;
}
