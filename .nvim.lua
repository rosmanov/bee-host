-- Project-local configuration for bee-host
-- Set up auto-formatting to use clang-format with GNU style, matching Neovim's editing indentation
vim.g.project_formatters_by_ft = {
  c = { "clang_format" },
  cpp = { "clang_format" },
}

vim.g.project_custom_formatters = {
  clang_format = {
    prepend_args = {
      "-style",
      -- GNU-compliant configuration matching cinoptions and indent rules
      "{BasedOnStyle: GNU, IndentWidth: 2, ColumnLimit: 79, UseTab: Never}"
    }
  }
}

-- Ensure LSP (clangd) formatting falls back to the exact same style when triggered manually
vim.g.project_lsp_servers = {
  clangd = {
    cmd = {
      "clangd",
      "--background-index",
      "--clang-tidy",
      "--fallback-style={BasedOnStyle: GNU, IndentWidth: 2, ColumnLimit: 79, UseTab: Never}"
    }
  }
}

-- Neovim editor configuration
local function apply_gnu_settings()
  -- Disable Treesitter indentation only for this project's buffers
  vim.opt_local.indentexpr = ""

  -- GNU cindent settings
  local opt = vim.opt_local
  opt.cindent = true
  opt.cinoptions = ">4,n-2,{2,^-2,:2,=2,g0,h2,p5,t0,+2,(0,u0,w1,m1"
  opt.shiftwidth = 2
  opt.softtabstop = 2
  opt.textwidth = 79

  -- Format Options: remove 'r' and 'o' flags, append 'c', 'q', 'l' flags
  opt.formatoptions:remove({ "r", "o" })
  opt.formatoptions:append("c")
  opt.formatoptions:append("q")
  opt.formatoptions:append("l")

  -- Code folding
  opt.foldmethod = "marker"
end

-- Safely run configuration without affecting other Neovim tasks
pcall(function()
  -- Apply immediately if the current buffer is already a C/C++ file
  if vim.tbl_contains({ "c", "cpp" }, vim.bo.filetype) then
    vim.schedule(apply_gnu_settings)
  end

  -- Set up local autocommand for any C/C++ files opened during this session
  local group = vim.api.nvim_create_augroup("ProjectCConfig", { clear = true })
  vim.api.nvim_create_autocmd("FileType", {
    group = group,
    pattern = { "c", "cpp" },
    callback = function()
      -- vim.schedule defers execution, guaranteeing that our settings
      -- run AFTER Treesitter finishes loading and sets its indentexpr.
      vim.schedule(apply_gnu_settings)
    end,
  })
end)

-- Warn if clang-format is not installed anywhere (system PATH or Mason)
local function check_clang_format()
  local has_system = vim.fn.executable("clang-format") == 1
  local mason_path = vim.fn.stdpath("data") .. "/mason/bin/clang-format"
  local has_mason = vim.fn.executable(mason_path) == 1

  if not has_system and not has_mason then
    vim.api.nvim_create_autocmd("FileType", {
      pattern = { "c", "cpp" },
      callback = function()
        vim.schedule(function()
          vim.notify(
            "Project formatters are configured, but 'clang-format' was not found.\n" ..
            "Install it via ':MasonInstall clang-format' or your system package manager for auto-formatting on save.",
            vim.log.levels.WARN,
            { title = "Project bee-host" }
          )
        end)
      end,
      once = true, -- Only warn once per session
    })
  end
end

pcall(check_clang_format)
