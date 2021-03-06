/**
 * Tencent is pleased to support the open source community by making Tars available.
 *
 * Copyright (C) 2016THL A29 Limited, a Tencent company. All rights reserved.
 *
 * Licensed under the BSD 3-Clause License (the "License"); you may not use this file except 
 * in compliance with the License. You may obtain a copy of the License at
 *
 * https://opensource.org/licenses/BSD-3-Clause
 *
 * Unless required by applicable law or agreed to in writing, software distributed 
 * under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR 
 * CONDITIONS OF ANY KIND, either express or implied. See the License for the 
 * specific language governing permissions and limitations under the License.
 */

#include "AServantImp.h"
#include "AServer.h"
#include "servant/Application.h"
#include "servant/Communicator.h"

using namespace std;
using namespace tars;

//////////////////////////////////////////////////////
class BServantCallback : public BServantPrxCallback
{

public:
    BServantCallback(TarsCurrentPtr &current)
    : _current(current)
    {}

    BServantCallback(TarsCurrentPtr &current, const tars::Promise<std::string> &promise)
    : _current(current)
    , _promise(promise)
    {}

    void callback_queryResult(tars::Int32 ret, const std::string &sOut)
    {
        if(ret == 0)
        {
            _promise.setValue(sOut);
        }
        else
        {
            handExp("callback_queryResult", ret);
        }
    }
    void callback_queryResult_exception(tars::Int32 ret)
    {
        handExp("callback_queryResult_exception", ret);
    }

private:
    void handExp(const std::string &sFuncName, tars::Int32 ret)
    {
        string s("sFuncName:");
        s += sFuncName;
        s += "|ret:";
        s += TC_Common::tostr(ret);

        _promise.setException(tars::copyException(s, -1));

        TLOGDEBUG("ServerPrxCallback handExp:" << s << endl);
    }

private:

    TarsCurrentPtr                    _current;
    tars::Promise<std::string>    _promise;
};
//////////////////////////////////////////////////////
class CServantCallback : public CServantPrxCallback
{

public:
    CServantCallback(TarsCurrentPtr &current)
    : _current(current)
    {}

    CServantCallback(TarsCurrentPtr &current, const tars::Promise<std::string> &promise)
    : _current(current)
    , _promise(promise)
    {}

    void callback_queryResult(tars::Int32 ret, const std::string &sOut)
    {
        if(ret == 0)
        {
            _promise.setValue(sOut);
        }
        else
        {
            handExp("callback_queryResult", ret);
        }
    }
    void callback_queryResult_exception(tars::Int32 ret)
    {
        handExp("callback_queryResult_exception", ret);
    }

private:
    void handExp(const std::string &sFuncName, tars::Int32 ret)
    {
        string s("sFuncName:");
        s += sFuncName;
        s += "|ret:";
        s += TC_Common::tostr(ret);

        _promise.setException(tars::copyException(s, -1));

        TLOGDEBUG("ServerPrxCallback handExp:" << s << endl);
    }

private:

    TarsCurrentPtr                    _current;
    tars::Promise<std::string>    _promise;
};
//////////////////////////////////////////////////////
tars::Future<std::string> sendBReq(BServantPrx prx, const std::string& sIn, tars::TarsCurrentPtr current)
{
    tars::Promise<std::string> promise;

    Test::BServantPrxCallbackPtr cb = new BServantCallback(current, promise);

    prx->async_queryResult(cb, sIn);

    return promise.getFuture();
}
//////////////////////////////////////////////////////
tars::Future<std::string> sendCReq(CServantPrx prx, const std::string& sIn, tars::TarsCurrentPtr current)
{
    tars::Promise<std::string> promise;

    Test::CServantPrxCallbackPtr cb = new CServantCallback(current, promise);

    prx->async_queryResult(cb, sIn);

    return promise.getFuture();
}
//////////////////////////////////////////////////////
tars::Future<std::string> handleBRspAndSendCReq(CServantPrx prx, TarsCurrentPtr current, const tars::Future<std::string>& future)
{
    std::string sResult("");
    std::string sException("");
    try 
    {
        sResult = future.get();

        return sendCReq(prx, sResult, current);
    } 
    catch (exception& e) 
    {
        TLOGDEBUG("Exception:" << e.what() << endl);
        sException = e.what();
    }

    tars::Promise<std::string> promise;
    promise.setValue(sException);

    return promise.getFuture();
}
//////////////////////////////////////////////////////
int handleCRspAndReturnClient(TarsCurrentPtr current, const tars::Future<std::string>& future)
{
    int ret = 0;
    std::string sResult("");
    try 
    {
        sResult = future.get();
    } 
    catch (exception& e) 
    {
        ret = -1;
        sResult = e.what();

        TLOGDEBUG("Exception:" << e.what() << endl);
    }

    AServant::async_response_queryResultSerial(current, ret, sResult);

    return 0;
}
//////////////////////////////////////////////////////
int handleBCRspAndReturnClient(TarsCurrentPtr current, const tars::Future<std::tuple<tars::Future<std::string>, tars::Future<std::string> > >& allFuture)
{
    int ret = 0;
    std::string sResult("");
    try 
    {
        const std::tuple<tars::Future<std::string>, tars::Future<std::string> >& tupleFuture = allFuture.get();

        std::string sResult1 = std::get<0>(tupleFuture).get();
        std::string sResult2 = std::get<1>(tupleFuture).get();

        sResult = sResult1;
        sResult += "|";
        sResult += sResult2;
    } 
    catch (exception& e) 
    {
        ret = -1;
        sResult = e.what();

        TLOGDEBUG("Exception:" << e.what() << endl);
    }

    AServant::async_response_queryResultParallel(current, ret, sResult);

    return 0;
}
//////////////////////////////////////////////////////
void AServantImp::initialize()
{
    //initialize servant here:
    //...
    _pPrxB = Application::getCommunicator()->stringToProxy<BServantPrx>("Test.BServer.BServantObj");
    _pPrxC = Application::getCommunicator()->stringToProxy<CServantPrx>("Test.CServer.CServantObj");
}
//////////////////////////////////////////////////////
void AServantImp::destroy()
{
}
//////////////////////////////////////////////////////
tars::Int32 AServantImp::queryResultSerial(const std::string& sIn, std::string &sOut, tars::TarsCurrentPtr current)
{
    current->setResponse(false);

    tars::Future<std::string> f = sendBReq(_pPrxB, sIn, current);

    f.then(tars::Bind(&handleBRspAndSendCReq, _pPrxC, current)).then(tars::Bind(&handleCRspAndReturnClient, current));

    return 0;
}
//////////////////////////////////////////////////////
tars::Int32 AServantImp::queryResultParallel(const std::string& sIn, std::string &sOut, tars::TarsCurrentPtr current)
{
    current->setResponse(false);

    tars::Future<std::string> f1 = sendBReq(_pPrxB, sIn, current);

    tars::Future<std::string> f2 = sendCReq(_pPrxC, sIn, current);

    tars::Future<std::tuple<tars::Future<std::string>, tars::Future<std::string> > > f_all = tars::WhenAll(f1, f2);

    f_all.then(tars::Bind(&handleBCRspAndReturnClient, current));

    return 0;
}

