# Contributing to Windows AI Agent

We love your input! We want to make contributing to Windows AI Agent as easy and transparent as possible, whether it's:

- Reporting a bug
- Discussing the current state of the code
- Submitting a fix
- Proposing new features
- Becoming a maintainer

## Development Process

We use GitHub to host code, to track issues and feature requests, as well as accept pull requests.

## Pull Requests

Pull requests are the best way to propose changes to the codebase. We actively welcome your pull requests:

1. Fork the repo and create your branch from `main`.
2. If you've added code that should be tested, add tests.
3. If you've changed APIs, update the documentation.
4. Ensure the test suite passes.
5. Make sure your code lints.
6. Issue that pull request!

## Any contributions you make will be under the MIT Software License

In short, when you submit code changes, your submissions are understood to be under the same [MIT License](http://choosealicense.com/licenses/mit/) that covers the project. Feel free to contact the maintainers if that's a concern.

## Report bugs using GitHub's [issue tracker](https://github.com/yourusername/windows-ai-agent/issues)

We use GitHub issues to track public bugs. Report a bug by [opening a new issue](https://github.com/yourusername/windows-ai-agent/issues/new).

## Write bug reports with detail, background, and sample code

**Great Bug Reports** tend to have:

- A quick summary and/or background
- Steps to reproduce
  - Be specific!
  - Give sample code if you can
- What you expected would happen
- What actually happens
- Notes (possibly including why you think this might be happening, or stuff you tried that didn't work)

## Development Setup

1. **Clone the repository**:

   ```bash
   git clone https://github.com/yourusername/windows-ai-agent.git
   cd windows-ai-agent
   ```

2. **Follow the build instructions** in [BUILD.md](BUILD.md)

3. **Set up your development environment**:
   - Install Visual Studio or VS Code with C++ extensions
   - Configure your API keys in local config files
   - Test both basic and advanced versions

## Code Style

- Use consistent indentation (4 spaces)
- Follow existing naming conventions
- Keep functions focused and well-documented
- Add comments for complex logic
- Use meaningful variable and function names

## Security Considerations

When contributing, please keep in mind:

- Never commit API keys or sensitive data
- Be cautious with system command execution
- Test safety features thoroughly
- Follow the principle of least privilege
- Consider the security implications of new features

## Feature Development Guidelines

### For new features:

1. **Discuss first**: Open an issue to discuss the feature before implementing
2. **Start small**: Implement a minimal version first
3. **Test thoroughly**: Ensure new features work in all execution modes
4. **Document**: Update README and other docs as needed
5. **Safety first**: Consider security implications

### Areas for contribution:

- **New AI providers**: Support for other AI APIs beyond Gemini
- **Enhanced safety**: Better command filtering and sandboxing
- **UI improvements**: GUI interface for the agent
- **Platform support**: macOS and Linux compatibility
- **Performance**: Optimization and caching improvements
- **Testing**: Unit tests and integration tests

## Testing

While we don't have a formal test suite yet, please test your changes by:

1. Building both agent versions
2. Testing with various command types
3. Verifying safety features work
4. Testing configuration changes
5. Checking error handling

## Documentation

When making changes:

- Update README.md if you change functionality
- Update BUILD.md if you change build requirements
- Add entries to CHANGELOG.md for user-facing changes
- Comment your code, especially complex algorithms

## Licensing

By contributing, you agree that your contributions will be licensed under the MIT License.

## Questions?

Feel free to open an issue for any questions about contributing!
