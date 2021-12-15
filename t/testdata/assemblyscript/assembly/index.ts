export * from "@solo-io/proxy-runtime/proxy"; // this exports the required functions for the proxy to interact with us.
//this.setEffectiveContext(callback.origin_context_id);
import {
  RootContext, Context, registerRootContext, log,
  LogLevelValues, FilterHeadersStatusValues,
  FilterTrailersStatusValues, GrpcStatusValues,
  send_local_response
} from "@solo-io/proxy-runtime";

class FaultInjection extends RootContext {
  status: i32 = 403;
  body: string = "";
  createContext(context_id: u32): Context {
    log(LogLevelValues.info, "FaultInjection createContext");
    return new FaultInjector(context_id, this);
  }
  onConfigure(configuration_size: u32): bool {
    let ok = super.onConfigure(configuration_size);
    if (!ok) {
        return false;
    }
    this.body = this.configuration_;
    return true;
  }
}

class FaultInjector extends Context {
  constructor(context_id: u32, root_context: FaultInjection) {
    super(context_id, root_context);
  }

  onRequestHeaders(a: u32, end_of_stream: bool): FilterHeadersStatusValues {
    log(LogLevelValues.info, "onRequestHeaders called!");
    let root_context = this.root_context as FaultInjection;
    log(LogLevelValues.info, root_context.body);
    send_local_response(root_context.status, "", String.UTF8.encode(root_context.body), [], GrpcStatusValues.Internal);

    return FilterHeadersStatusValues.StopIteration;
  }

}

registerRootContext((context_id: u32) => {
    return new FaultInjection(context_id);
}, "FaultInjection");
