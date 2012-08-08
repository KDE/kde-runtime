/****************************************************************************
**
** Copyright (C) 2011 Marco Martin  <mart@kde.org>
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Components project.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Nokia Corporation and its Subsidiary(-ies) nor
**     the names of its contributors may be used to endorse or promote
**     products derived from this software without specific prior written
**     permission.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
** $QT_END_LICENSE$
**
****************************************************************************/

// The PageStack item defines a container for pages and a stack-based
// navigation model. Pages can be defined as QML items or components.

/**Documented API
Inherits:
        Item

Imports:
        QtQuick 1.1
        .
        PageStack.js

Description:
        The PageStack component provides a stack-based navigation model that you can use in your application. A stack-based navigation model means that a page of content for your application is pushed onto a stack when the user navigates deeper into the application page hierarchy. The user can then go back to the previous page (or several pages back) by popping a page (or several pages) off the top of the stack.

Properties:
      * int depth:
        The number of pages on the page stack.

      * Item currentPage:
        The page in the stack that is currently visible.

      * ToolBar toolBar:
        The toolbar container for the tools associated with each page. If a toolbar is specified, the tools set for the current page is shown to the user.
        If toolbar is null, then no tools are shown even if a page does have tools.

      * variant initialPage:
        The page to be automatically loaded when this PageStack component gets instantiated.

      * bool busy:
        Indicates whether there is an ongoing page transition.

Methods:
      * clear():
        Clears the page stack of all pages.

      * find(function):
        This iterates, top to bottom, through all the pages in the page stack and passes each page to the given function. If the specified function returns true, the iterating stops and this function returns the page that produced the true result. If no matching page is found in the page stack, null is returned.

      * pop(page, immediate):
        When you use pop() with no arguments, it pops the top page off the stack and returns that page to the caller. The normal pop transition animation is performed. If the page popped off the stack is based on the Item element, the page is re-parented back to its original parent. 
        If the page didn't have an original parent (ie, was created with push(Qt.createComponent("foo.qml")) the page instance will be deleted.
        If you give a page argument, the stack is unwound to the given page. Any Item-based pages popped off the stack are re-parented to their original parent.
        If the given page is not found in the stack, the stack is unwound to the first page in the stack. However, if you specifically want to unwind the page stack to the first page in the stack, it is best to be explicit about what you are doing and use pop(null) rather than guessing a page that is not on the stack.
        The immediate argument defaults to false which means the normal transition animation is performed when a page is popped. If you do not want the transition animation to be performed pass a value of true for immediate.
        Note: A pop() on a stack with that contains 0 or 1 pages is a no-operation.
        Returns: The page that was top-most on the stack before the pop operation.

      * push(page, properties, immediate):
        Pushes the given page onto the page stack. You can use a component, item or string for the page. If the page is based on the Item element, the page is re-parented. If a string is used then it is interpreted as a URL that is used to load a page component. The push operation results in the appropriate transition animation being run. If you are pushing an array of pages, the transition animation is only shown for the last page.
        Returns: The new top page on the stack.
        The page argument can also be an array of pages. In this case, all the pages in the array are pushed onto the stack. The items in the array can be components, items or strings just like for pushing a single page.
        The page argument can also be an object that specifies a page with properties or even an array of pages with properties.
        The properties argument is optional and allows you to specify values for properties in the page being pushed.
        The immediate argument defaults to false which means the normal transition animation is performed when a page is pushed. If you do not want the transition animation to be performed pass a value of true for immediate.
        Note: When the stack is empty, a push() or replace() does not perform a transition animation because there is no page to transition from. The only time this normally happens is when an application is starting up so it is not appropriate to have a transition animation anyway.
        See also Page.

      * replace(page, properties, immediate):
        Replaces the top-most page on the stack with page. As in the push() operation, you can use a component, item or string for the page, or even an array of pages. If the page is based on the Item element, the page is re- parented. As in the pop() operation, the replaced page on the stack is re- parented back to its original parent.
        Returns: The new top page on the stack.
        See also push().

**/

import QtQuick 1.1
import "." 0.1
import "private/PageStack.js" as Engine

Item {
    id: root

    width: parent ? parent.width : 0
    height: parent ? parent.height : 0

    property int depth: Engine.getDepth()
    property Item currentPage: null
    property ToolBar toolBar
    property variant initialPage

    // Indicates whether there is an ongoing page transition.
    property bool busy: internal.ongoingTransitionCount > 0

    // Pushes a page on the stack.
    // The page can be defined as a component, item or string.
    // If an item is used then the page will get re-parented.
    // If a string is used then it is interpreted as a url that is used to load a page component.
    //
    // The page can also be given as an array of pages. In this case all those pages will be pushed
    // onto the stack. The items in the stack can be components, items or strings just like for single
    // pages. Additionally an object can be used, which specifies a page and an optional properties
    // property. This can be used to push multiple pages while still giving each of them properties.
    // When an array is used the transition animation will only be to the last page.
    //
    // The properties argument is optional and allows defining a map of properties to set on the page.
    // If the immediate argument is true then no transition animation is performed.
    // Returns the page instance.
    function push(page, properties, immediate)
    {
        return Engine.push(page, properties, false, immediate);
    }

    // Pops a page off the stack.
    // If page is specified then the stack is unwound to that page, to unwind to the first page specify
    // page as null. If the immediate argument is true then no transition animation is performed.
    // Returns the page instance that was popped off the stack.
    function pop(page, immediate)
    {
        return Engine.pop(page, immediate);
    }

    // Replaces a page on the stack.
    // See push() for details.
    function replace(page, properties, immediate)
    {
        return Engine.push(page, properties, true, immediate);
    }

    // Clears the page stack.
    function clear()
    {
        return Engine.clear();
    }

    // Iterates through all pages (top to bottom) and invokes the specified function.
    // If the specified function returns true the search stops and the find function
    // returns the page that the iteration stopped at. If the search doesn't result
    // in any page being found then null is returned.
    function find(func)
    {
        return Engine.find(func);
    }

    // Called when the page stack visibility changes.
    onVisibleChanged: {
        if (currentPage) {
            internal.setPageStatus(currentPage, visible ? PageStatus.Active : PageStatus.Inactive);
            if (visible)
                currentPage.visible = currentPage.parent.visible = true;
        }
    }

    onInitialPageChanged: {
        if (!internal.completed) {
            return
        }

        if (initialPage) {
            if (depth == 0) {
                push(initialPage, null, true)
            } else if (depth == 1) {
                replace(initialPage, null, true)
            } else {
                console.log("Cannot update PageStack.initialPage")
            }
        }
    }

    Component.onCompleted: {
        internal.completed = true
        if (initialPage && depth == 0)
            push(initialPage, null, true)
    }

    QtObject {
        id: internal

        // The number of ongoing transitions.
        property int ongoingTransitionCount: 0

        //FIXME: there should be a way to access to theh without storing it in an ugly way
        property bool completed: false

        // Sets the page status.
        function setPageStatus(page, status)
        {
            if (page != null) {
                if (page.status !== undefined) {
                    if (status == PageStatus.Active && page.status == PageStatus.Inactive)
                        page.status = PageStatus.Activating;
                    else if (status == PageStatus.Inactive && page.status == PageStatus.Active)
                        page.status = PageStatus.Deactivating;

                    page.status = status;
                }
            }
        }
    }

    // Component for page containers.
    Component {
        id: containerComponent

        Item {
            id: container

            width: parent ? parent.width : 0
            height: parent ? parent.height : 0

            // The actual parent of page: page will anchor to that
            // in this case is container itself, not the case for PageRow
            property Item pageParent: container

            // The states correspond to the different possible positions of the container.
            state: "Hidden"

            // The page held by this container.
            property Item page: null

            // The owner of the page.
            property Item owner: null

            // The width of the longer stack dimension
            property int stackWidth: Math.max(root.width, root.height)

            // Duration of transition animation (in ms)
            property int transitionDuration: 250

            // Flag that indicates the container should be cleaned up after the transition has ended.
            property bool cleanupAfterTransition: false

            // Flag that indicates if page transition animation is running
            property bool transitionAnimationRunning: false

            // State to be set after previous state change animation has finished
            property string pendingState: "none"

            // Ensures that transition finish actions are executed
            // in case the object is destroyed before reaching the
            // end state of an ongoing transition
            Component.onDestruction: {
                if (transitionAnimationRunning)
                    transitionEnded();
            }

            // Sets pending state as current if state change is delayed
            onTransitionAnimationRunningChanged: {
                if (!transitionAnimationRunning && pendingState != "none") {
                    state = pendingState;
                    pendingState = "none";
                }
            }

            // Handles state change depening on transition animation status
            function setState(newState)
            {
                if (transitionAnimationRunning)
                    pendingState = newState;
                else
                    state = newState;
            }

            // Performs a push enter transition.
            function pushEnter(immediate, orientationChanges)
            {
                if (!immediate) {
                    if (orientationChanges)
                        setState("LandscapeRight");
                    else
                        setState("Right");
                }
                setState("");
                page.visible = true;
                if (root.visible && immediate)
                    internal.setPageStatus(page, PageStatus.Active);
            }

            // Performs a push exit transition.
            function pushExit(replace, immediate, orientationChanges)
            {
                if (orientationChanges)
                    setState(immediate ? "Hidden" : "LandscapeLeft");
                else
                    setState(immediate ? "Hidden" : "Left");
                if (root.visible && immediate)
                    internal.setPageStatus(page, PageStatus.Inactive);
                if (replace) {
                    if (immediate)
                        cleanup();
                    else
                        cleanupAfterTransition = true;
                }
            }

            // Performs a pop enter transition.
            function popEnter(immediate, orientationChanges)
            {
                if (!immediate)
                    state = orientationChanges ? "LandscapeLeft" : "Left";
                setState("");
                page.visible = true;
                if (root.visible && immediate)
                    internal.setPageStatus(page, PageStatus.Active);
            }

            // Performs a pop exit transition.
            function popExit(immediate, orientationChanges)
            {
                if (orientationChanges)
                    setState(immediate ? "Hidden" : "LandscapeRight");
                else
                    setState(immediate ? "Hidden" : "Right");

                if (root.visible && immediate)
                    internal.setPageStatus(page, PageStatus.Inactive);
                if (immediate)
                    cleanup();
                else
                    cleanupAfterTransition = true;
            }

            // Called when a transition has started.
            function transitionStarted()
            {
                transitionAnimationRunning = true;
                internal.ongoingTransitionCount++;
                if (root.visible) {
                    internal.setPageStatus(page, (state == "") ? PageStatus.Activating : PageStatus.Deactivating);
                }
            }

            // Called when a transition has ended.
            function transitionEnded()
            {
                if (state != "")
                    state = "Hidden";
                if (root.visible)
                    internal.setPageStatus(page, (state == "") ? PageStatus.Active : PageStatus.Inactive);

                internal.ongoingTransitionCount--;
                transitionAnimationRunning = false;
                if (cleanupAfterTransition)
                    cleanup();
            }

            states: [
                // Explicit properties for default state.
                State {
                    name: ""
                    PropertyChanges { target: container; visible: true; opacity: 1 }
                },
                // Start state for pop entry, end state for push exit.
                State {
                    name: "Left"
                    PropertyChanges { target: container; x: -width / 2; opacity: 0 }
                },
                // Start state for pop entry, end state for push exit
                // when exiting portrait and entering landscape.
                State {
                    name: "LandscapeLeft"
                    PropertyChanges { target: container; x: -stackWidth / 2; opacity: 0 }
                },
                // Start state for push entry, end state for pop exit.
                State {
                    name: "Right"
                    PropertyChanges { target: container; x: width / 2; opacity: 0 }
                },
                // Start state for push entry, end state for pop exit
                // when exiting portrait and entering landscape.
                State {
                    name: "LandscapeRight"
                    PropertyChanges { target: container; x: stackWidth / 2; opacity: 0 }
                },
                // Inactive state.
                State {
                    name: "Hidden"
                    PropertyChanges { target: container; visible: false }
                }
            ]

            transitions: [
                // Push exit transition
                Transition {
                    from: ""; to: "Left"
                    SequentialAnimation {
                        ScriptAction { script: transitionStarted() }
                        ParallelAnimation {
                            PropertyAnimation { properties: "x"; easing.type: Easing.InQuad; duration: transitionDuration }
                            PropertyAnimation { properties: "opacity"; easing.type: Easing.Linear; duration: transitionDuration }
                        }
                        ScriptAction { script: transitionEnded() }
                    }
                },
                // Pop entry transition
                Transition {
                    from: "Left"; to: ""
                    SequentialAnimation {
                        ScriptAction { script: transitionStarted() }
                        ParallelAnimation {
                            PropertyAnimation { properties: "x"; easing.type: Easing.OutQuad; duration: transitionDuration }
                            PropertyAnimation { properties: "opacity"; easing.type: Easing.Linear; duration: transitionDuration }
                        }
                        ScriptAction { script: transitionEnded() }
                    }
                },
                // Push exit transition landscape
                Transition {
                    from: ""; to: "LandscapeLeft"
                    SequentialAnimation {
                        ScriptAction { script: transitionStarted() }
                        ParallelAnimation {
                            PropertyAnimation { properties: "x"; easing.type: Easing.InQuad; duration: transitionDuration }
                            PropertyAnimation { properties: "opacity"; easing.type: Easing.Linear; duration: transitionDuration }
                        }
                        ScriptAction { script: transitionEnded() }
                    }
                },
                // Pop entry transition landscape
                Transition {
                    from: "LandscapeLeft"; to: ""
                    SequentialAnimation {
                        ScriptAction { script: transitionStarted() }
                        ParallelAnimation {
                            PropertyAnimation { properties: "x"; easing.type: Easing.OutQuad; duration: transitionDuration }
                            PropertyAnimation { properties: "opacity"; easing.type: Easing.Linear; duration: transitionDuration }
                        }
                        ScriptAction { script: transitionEnded() }
                    }
                },
                // Pop exit transition
                Transition {
                    from: ""; to: "Right"
                    SequentialAnimation {
                        ScriptAction { script: transitionStarted() }
                        ParallelAnimation {
                            PropertyAnimation { properties: "x"; easing.type: Easing.InQuad; duration: transitionDuration }
                            PropertyAnimation { properties: "opacity"; easing.type: Easing.Linear; duration: transitionDuration }
                        }
                        // Workaround for transition animation bug causing ghost view with page pop transition animation
                        // TODO: Root cause still unknown
                        PropertyAnimation {}
                        ScriptAction { script: transitionEnded() }
                    }
                },
                // Push entry transition
                Transition {
                    from: "Right"; to: ""
                    SequentialAnimation {
                        ScriptAction { script: transitionStarted() }
                        ParallelAnimation {
                            PropertyAnimation { properties: "x"; easing.type: Easing.OutQuad; duration: transitionDuration }
                            PropertyAnimation { properties: "opacity"; easing.type: Easing.Linear; duration: transitionDuration }
                        }
                        ScriptAction { script: transitionEnded() }
                    }
                },
                // Pop exit transition landscape
                Transition {
                    from: ""; to: "LandscapeRight"
                    SequentialAnimation {
                        ScriptAction { script: transitionStarted() }
                        ParallelAnimation {
                            PropertyAnimation { properties: "x"; easing.type: Easing.InQuad; duration: transitionDuration }
                            PropertyAnimation { properties: "opacity"; easing.type: Easing.Linear; duration: transitionDuration }
                        }
                        // Workaround for transition animation bug causing ghost view with page pop transition animation
                        // TODO: Root cause still unknown
                        PropertyAnimation {}
                        ScriptAction { script: transitionEnded() }
                    }
                },
                // Push entry transition landscape
                Transition {
                    from: "LandscapeRight"; to: ""
                    SequentialAnimation {
                        ScriptAction { script: transitionStarted() }
                        ParallelAnimation {
                            PropertyAnimation { properties: "x"; easing.type: Easing.OutQuad; duration: transitionDuration }
                            PropertyAnimation { properties: "opacity"; easing.type: Easing.Linear; duration: transitionDuration }
                        }
                        ScriptAction { script: transitionEnded() }
                    }
                }
            ]

            // Cleans up the container and then destroys it.
            function cleanup()
            {
                if (page != null) {
                    if (page.status == PageStatus.Active) {
                        internal.setPageStatus(page, PageStatus.Inactive)
                    }
                    if (owner != container) {
                        // container is not the owner of the page - re-parent back to original owner
                        page.visible = false;
                        page.anchors.fill = undefined
                        page.parent = owner;
                    }
                }

                container.destroy();
            }
        }
    }
}

