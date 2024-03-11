#include "executable.hxx"

#include <filesystem>
#include <sys/wait.h>
#include <sys/poll.h>

Alchemist::System::Linux::Executable::Executable(const std::string& prog, const std::vector<std::string>& args):m_program(prog), m_arguments(args) {
	run();
}

Alchemist::System::Linux::Executable::Executable(std::string&& prog, std::vector<std::string>&& args):m_program(std::move(prog)), m_arguments(std::move(args)) {
	run();
}

Alchemist::System::Linux::Executable& Alchemist::System::Linux::Executable::operator>>(Executable& exe) {
	m_forwarder.reset(new std::thread(&Alchemist::System::Linux::Executable::consume_and_forward, this, std::ref(exe)));
	return exe;
}

std::optional<std::string>& Alchemist::System::Linux::Executable::operator>>(std::optional<std::string>& data) {
	m_pstdout >> data;
	return data;
}

std::ostream& Alchemist::System::Linux::operator<<(std::ostream& os, const Linux::Executable& exe) {
	std::optional<std::string> data;
	exe.m_pstdout >> data;
	if (data) os << *data;
	return os;
}

Alchemist::System::Linux::Executable& Alchemist::System::Linux::Executable::operator<<(const std::string& data) {
	m_pstdin << data;
	return *this;
}

void Alchemist::System::Linux::Executable::operator<<(const System::Executable::_EoF&) {
	m_pstdin.close_write();
}

void Alchemist::System::Linux::Executable::run() {
	m_pid = fork();

	if (m_pid == 0) {
		/* STDIN: Child reads from STDIN but does not write to */
		m_pstdin.close_write();
		m_pstdin.bind_read(STDIN_FILENO);

		/* STDOUT: Child writes to STDOUT but does not read from */
		m_pstdout.close_read();
		m_pstdout.bind_write(STDOUT_FILENO);

		/* STDERR: Child writes to STDERR but does not read from */
		m_pstdout.close_read();
		m_pstdout.bind_write(STDERR_FILENO);

		std::string program_file = std::filesystem::path(m_program).filename().string();
		std::vector<char*> argv;
		argv.reserve(m_arguments.size() + 2);
		argv.push_back(program_file.data());
		for (size_t i = 0; i < m_arguments.size(); i++)
			argv.push_back(m_arguments[i].data());
		argv.push_back(NULL);
		
		execvp(m_program.data(), argv.data());
		// If we reach here then we failed to execute the program
		exit(0);
	}
	else {
		/* STDIN: Parent writes to STDIN but does not read from */
		m_pstdin.close_read();

		/* STDOUT: Parent reads from to STDOUT but does not write to */
		m_pstdout.close_write();

		/* STDERR: Parent reads from to STDERR but does not write to */
		m_pstderr.close_write();
	}
}

void Alchemist::System::Linux::Executable::write(const std::string& str) {
	m_pstdin << str;
}

int Alchemist::System::Linux::Executable::wait() {
	int status;
	if (m_forwarder)
		m_forwarder->join();
	waitpid(m_pid, &status, 0);
	return status;
}

void Alchemist::System::Linux::Executable::consume_and_forward(Executable& exec) {
	do {
		std::optional<std::string> buffer;
		m_pstdout.poll(100);
		m_pstdout >> buffer;
		if (buffer)
			exec.m_pstdin << *buffer;
	} while (!m_pstdout.has_read_event(POLLHUP));
	exec.m_pstdin.close_write();
}