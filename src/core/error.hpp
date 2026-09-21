//===----------------------------------------------------------------------===//
//
// Part of the ROPsmith Project.
// See LICENSE for license information.
//
//===----------------------------------------------------------------------===//
//!
//! \file core/error.hpp
//! Declaration of the unified error reporting type and formatting helpers.
//!
//===----------------------------------------------------------------------===//

#ifndef CORE_ERROR_HPP
#define CORE_ERROR_HPP

#include <expected>
#include <format>
#include <source_location>
#include <string>
#include <system_error>

namespace core
{

//! \class Error
//! \brief Represents an operational or system error with call-site source tracking.
//!
//! Encapsulates a contextual error message, an optional standard error code,
//! and source location metadata captured automatically at the point of failure.
class Error
{
  public:
    //! \brief Constructs an error with a custom message and source location.
    //!
    //! \param msg Explanatory text describing the failure condition.
    //! \param loc Source code location where the error originated.
    explicit Error(
        std::string msg,
        std::source_location loc = std::source_location::current()
    ) noexcept
        : msg_(std::move(msg)), loc_(loc)
    {
    }

    //! \brief Constructs an error from an existing error code with contextual detail.
    //!
    //! \param ec The underlying system, OS, or library error code.
    //! \param msg Explanatory text providing high-level context for the code.
    //! \param loc Source code location where the error originated.
    Error(
        std::error_code ec,
        std::string msg,
        std::source_location loc = std::source_location::current()
    ) noexcept
        : code_(ec), msg_(std::move(msg)), loc_(loc)
    {
    }

    //! \brief Retrieves the contextual error message.
    //!
    //! \return Const reference to the explanatory message string.
    [[nodiscard]] const std::string &message() const noexcept
    {
        return this->msg_;
    }

    //! \brief Retrieves the underlying system error code.
    //!
    //! \return Const reference to the associated std::error_code.
    [[nodiscard]] const std::error_code &code() const noexcept
    {
        return this->code_;
    }

    //! \brief Retrieves the source code location where the error occurred.
    //!
    //! \return Const reference to the std::source_location record.
    [[nodiscard]] const std::source_location &location() const noexcept
    {
        return this->loc_;
    }

    //! \brief Generates a user-facing formatted error string.
    //!
    //! Produces a clean, non-technical diagnostic suitable for display
    //! in command-line interfaces.
    //!
    //! \return Formatted error message.
    [[nodiscard]] std::string user_message() const
    {
        if (code_)
        {
            return std::format("{}: {}", this->msg_, this->code_.message());
        }

        return this->msg_;
    }

    //! \brief Generates a complete diagnostic trace for debugging.
    //!
    //! Combines filename, line number, enclosing function name, error
    //! category, numeric code value, and the contextual description.
    //!
    //! \return Verbose execution and location trace.
    [[nodiscard]] std::string trace() const
    {
        if (this->code_)
        {
            return std::format(
                "{}:{}: in '{}' [{}:{}]: {}: {}",
                this->loc_.file_name(),
                this->loc_.line(),
                this->loc_.function_name(),
                this->code_.category().name(),
                this->code_.value(),
                this->msg_,
                this->code_.message()
            );
        }

        return std::format(
            "{}:{}: in '{}': {}",
            this->loc_.file_name(),
            this->loc_.line(),
            this->loc_.function_name(),
            this->msg_
        );
    }

    //! \brief Explicitly converts the error to a user-facing formatted string.
    //!
    //! Delegates to message() to provide a diagnostic.
    //!
    //! \return The formatted error string.
    [[nodiscard]] explicit operator std::string() const
    {
        return this->message();
    }

  private:
    std::error_code code_{};
    std::string msg_;
    std::source_location loc_;
};

//! \brief Type alias wrapping std::expected with the unified Error type.
//!
//! \tparam T The expected payload value on success.
template <typename T> using Result = std::expected<T, Error>;

} // namespace core

//! \brief Formatter specialization for std::format and std::print compatibility.
template <> struct std::formatter<core::Error> : std::formatter<std::string>
{
    //! \brief Formats a core::Error instance using its user-facing message.
    //!
    //! \param err The core::Error instance to format.
    //! \param ctx The formatting context.
    //! \return Iterator to the end of the formatted output.
    auto format(const core::Error &err, std::format_context &ctx) const
    {
        return std::formatter<std::string>::format(err.user_message(), ctx);
    }
};

#endif // CORE_ERROR_HPP
