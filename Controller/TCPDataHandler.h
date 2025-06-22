#ifndef TCPDataHandlerH
#define TCPDataHandlerH

#include <System.Classes.hpp>
#include <IdBaseComponent.hpp>
#include <IdComponent.hpp>
#include <IdTCPClient.hpp>
#include <IdTCPConnection.hpp>
#include <functional>

enum class TDataSourceType { Network, File };

class TCPDataHandler : public TObject
{
private:
    // --- 내부 스레드 클래스 선언 ---
    class TWorkerThread : public TThread
    {
    private:
        TCPDataHandler* FHandler; // 부모 핸들러에 접근하기 위함
    protected:
        // TThread의 순수 가상 함수를 반드시 구현해야 함
        void __fastcall Execute() override;
    public:
        __fastcall TWorkerThread(TCPDataHandler* AHandler);
    };
    // ---------------------------------

    friend class TWorkerThread; // 내부 클래스가 private 멤버에 접근할 수 있도록 허용

    // 내부 컴포넌트
    TIdTCPClient* FClient;
    TWorkerThread* FWorkerThread; // TThread -> TWorkerThread로 변경
    TStreamReader* FPlaybackStream;
    TStreamWriter* FRecordStream;
    TComponent* FOwner;

    // 상태 변수
    String FHost;
    int FPort;
    String FPlaybackFileName;
    TDataSourceType FDataSourceType;
    bool FIsActive;

    // 동기화 메소드 (변경 없음)
    void __fastcall SyncNotifyData(AnsiString data);
    void __fastcall SyncNotifyConnected();
    void __fastcall SyncNotifyReconnecting();
    void __fastcall ThreadTerminated(TObject* Sender);

public:
    __fastcall TCPDataHandler(TComponent* AOwner);
    __fastcall ~TCPDataHandler();

    typedef std::function<void(const String& data)> TDataCallback;
    typedef std::function<void()> TNotifyEventCallback;
    typedef std::function<void(const String& reason)> TDisconnectedCallback;
    typedef std::function<void()> TReconnectingCallback;

    TDataCallback OnDataReceived;
    TNotifyEventCallback OnConnected;
    TDisconnectedCallback OnDisconnected;
    TReconnectingCallback OnReconnecting;

    void Connect(const String& host, int port);
    void StartPlayback(const String& fileName);
    void Disconnect();
    void StartRecording(const String& fileName);
    void StopRecording();

    bool IsActive() { return FIsActive; }
};

#endif