#pragma once

namespace scripts_proxy {

[[nodiscard]] bool IsMsmrExecutableBasename(
    const wchar_t* basename) noexcept;
[[nodiscard]] bool IsMsmrHostProcess() noexcept;

} // namespace scripts_proxy
