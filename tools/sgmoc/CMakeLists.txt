set(LLVM_LINK_COMPONENTS Support)

add_clang_executable(sgmoc 
    main.cpp
    sgASTConsumer.cpp
    sgPPCallbacks.cpp
)

target_link_libraries(sgmoc
    clangAST
    clangASTMatchers
    clangBasic
    clangFrontend
    clangLex
    clangRewrite
    clangTooling
    clangToolingCore
)



