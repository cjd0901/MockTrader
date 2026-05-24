cmake_minimum_required(VERSION 3.22)
project(MockTraderClient LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Gui Widgets Network Charts)

qt_standard_project_setup()

add_executable(mock_trader_client
    resources/resources.qrc

    # app shell
    src/main.cpp
    src/MainWindow.cpp
    src/MainWindow.h

    # application config & theme
    src/app/KlineLoadConfig.h
    src/app/ServerConfig.cpp
    src/app/ServerConfig.h
    src/app/AppBranding.cpp
    src/app/AppBranding.h
    src/app/AppStyle.cpp
    src/app/AppStyle.h

    # domain & wire protocols
    src/model/MarketTypes.h
    src/protocol/TcpCandleCodec.cpp
    src/protocol/TcpCandleCodec.h
    src/api/HttpApiUrl.cpp
    src/api/HttpApiUrl.h
    src/api/HttpBacktestClient.cpp
    src/api/HttpBacktestClient.h
    src/api/HttpStockClient.cpp
    src/api/HttpStockClient.h
    src/api/TcpCandleClient.cpp
    src/api/TcpCandleClient.h

    # UI
    src/pages/HomePage.cpp
    src/pages/HomePage.h
    src/pages/StockDetailPage.cpp
    src/pages/StockDetailPage.h
    src/widgets/KlineTimelineBar.cpp
    src/widgets/KlineTimelineBar.h
    src/widgets/BacktestPanel.cpp
    src/widgets/BacktestPanel.h
    src/widgets/StrategyPicker.cpp
    src/widgets/StrategyPicker.h
    src/widgets/StockListDelegate.cpp
    src/widgets/StockListDelegate.h
)

target_link_libraries(mock_trader_client PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
    Qt6::Charts
)

target_include_directories(mock_trader_client PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

if(MSVC)
    target_compile_options(mock_trader_client PRIVATE /W4 /permissive-)
else()
    target_compile_options(mock_trader_client PRIVATE -Wall -Wextra -Wpedantic)
endif()
